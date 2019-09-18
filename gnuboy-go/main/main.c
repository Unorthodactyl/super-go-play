#pragma GCC optimize ("O3")

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_task_wdt.h"
#include "esp_spiffs.h"
#include "driver/rtc_io.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"

#include "../components/gnuboy/loader.h"
#include "../components/gnuboy/hw.h"
#include "../components/gnuboy/lcd.h"
#include "../components/gnuboy/fb.h"
#include "../components/gnuboy/cpu.h"
#include "../components/gnuboy/mem.h"
#include "../components/gnuboy/sound.h"
#include "../components/gnuboy/pcm.h"
#include "../components/gnuboy/regs.h"
#include "../components/gnuboy/rtc.h"
#include "../components/gnuboy/gnuboy.h"

#include <string.h>

#include "hourglass_empty_black_48dp.h"

#include "../components/odroid/odroid_settings.h"
#include "../components/odroid/odroid_input.h"
#include "../components/odroid/odroid_display.h"
#include "../components/odroid/odroid_audio.h"
#include "../components/odroid/odroid_system.h"
#include "../components/odroid/odroid_sdcard.h"


extern int debug_trace;

struct fb fb;
struct pcm pcm;
extern struct scan scan;

int interlace = 0;
int skipFrame = 0;

uint16_t* displayBuffer[2]; //= { fb0, fb0 }; //[160 * 144];
uint8_t currentBuffer; // index for display_buffer
uint16_t* framebuffer; // pointer to currentBuffer

int frame = 0;
uint elapsedTime = 0;

int32_t* audioBuffer[2];
volatile uint8_t currentAudioBuffer = 0;
volatile uint16_t currentAudioSampleCount;
volatile int16_t* currentAudioBufferPtr;

odroid_battery_state battery_state;

const char* StateFileName = "/storage/gnuboy.sav";

#define GAMEBOY_WIDTH (160)
#define GAMEBOY_HEIGHT (144)

#define PIXEL_MASK 0x3F

struct update_meta {
	odroid_scanline diff[GAMEBOY_HEIGHT];
	bool skip_frame;
	uint8_t *buffer;
	int stride;
    uint16_t palette[64 * 2];
};	

static struct update_meta update1 = {0,};
static struct update_meta update2 = {0,};
static struct update_meta *update = &update2;
static struct update_meta *old_update;
//int update_palette_dirty = true;

#define AUDIO_SAMPLE_RATE (32000)

const char* SD_BASE_PATH = "/sd";

// --- MAIN
QueueHandle_t vidQueue;
QueueHandle_t audioQueue;

float Volume = 1.0f;

int pcm_submit()
{
    odroid_audio_submit(currentAudioBufferPtr, currentAudioSampleCount / 2);
    return 1;
}


int BatteryPercent = 100;

void run_to_vblank()
{
  /* FRAME BEGIN */

  /* FIXME: djudging by the time specified this was intended
  to emulate through vblank phase which is handled at the
  end of the loop. */
  cpu_emulate(2280);

  /* FIXME: R_LY >= 0; comparsion to zero can also be removed
  altogether, R_LY is always 0 at this point */
  while (R_LY > 0 && R_LY < 144)
  {
    /* Step through visible line scanning phase */
    emu_step();
  }

  /* VBLANK BEGIN */
  if (!skipFrame) {
      old_update = update;
	  
      // Swap updates
	  update = (update == &update1) ? &update2 : &update1;

	  update->buffer = framebuffer;
	  update->stride = fb.pitch;
      memcpy(update->palette, scan.pal2, 64 * sizeof(uint16_t));
      
	  // Diff framebuffers and send the update to video task
	  // TODO: Somehow determine when to interlace properly
	  odroid_buffer_diff(update->buffer, old_update->buffer, 
	  hasPaletteUpdate ? update->palette : NULL, 
      hasPaletteUpdate ? old_update->palette : NULL,
      GAMEBOY_WIDTH, GAMEBOY_HEIGHT,
      update->stride, PIXEL_MASK, 0, update->diff);
	  xQueueSend(vidQueue, &update, portMAX_DELAY);
	  
      // Swap framebuffers
	  currentBuffer = currentBuffer ? 0 : 1;
	  framebuffer = displayBuffer[currentBuffer];
	  fb.ptr = framebuffer;

      //flip update queue again
      update = (update == &update1) ? &update2 : &update1;
  }
  rtc_tick();
  sound_mix();

  //if (pcm.pos > 100)
  {
        currentAudioBufferPtr = audioBuffer[currentAudioBuffer];
        currentAudioSampleCount = pcm.pos;

        void* tempPtr = 0x1234;
        xQueueSend(audioQueue, &tempPtr, portMAX_DELAY);

        // Swap buffers
        currentAudioBuffer = currentAudioBuffer ? 0 : 1;
        pcm.buf = audioBuffer[currentAudioBuffer];
        pcm.pos = 0;
  }

  if (!(R_LCDC & 0x80)) {
    /* LCDC operation stopped */
    /* FIXME: djudging by the time specified, this is
    intended to emulate through visible line scanning
    phase, even though we are already at vblank here */
    cpu_emulate(32832);
  }

  while (R_LY > 0) {
    /* Step through vblank phase */
    emu_step();
  }
}


uint16_t* menuFramebuffer = 0;

volatile bool videoTaskIsRunning = false;
bool scaling_enabled = true;
bool previous_scale_enabled = true;

void IRAM_ATTR videoTask(void *arg)
{
  esp_err_t ret;

  videoTaskIsRunning = true;
  struct update_meta *update = NULL;
  uint16_t display_palette[64] = {0, };

  while(1)
  {
        xQueuePeek(vidQueue, &update, portMAX_DELAY);

        if (update == 1)
			break;
		
		bool scale_changed = (previous_scale_enabled != scaling_enabled);

        if (scale_changed)
        {
            ili9341_blank_screen();
            previous_scale_enabled = scaling_enabled;
            if (scaling_enabled) {
				// TODO: Scaling looks kinda ugly compared to old gnuboy, not sure how to fix that
                odroid_display_reset_scale(GAMEBOY_WIDTH, GAMEBOY_HEIGHT);
            } else {
                odroid_display_set_scale(GAMEBOY_WIDTH, GAMEBOY_HEIGHT, 1.2f);
            }
        }
		
		// TODO: For palette diffing scan.pal2 probably needs to get changed
		//       to a buffered thing. Maybe change update_meta to contain a the palette?
		bool full_update = scale_changed;
		//if (update_palette_dirty) {
		//	update_palette_dirty = 0;
		//	full_update = true;
		//}
        
        if (hasPaletteUpdate)
        {
			//borrowed from Paspartout's fork
            for(int i = 0; i < PIXEL_MASK+1; i++) {
            const uint16_t pix = update->palette[i];
            display_palette[i] =  pix << 8 | pix >> 8;
            }
            hasPaletteUpdate = false;
        }
        
        
		ili9341_write_frame_8bit(update->buffer, full_update || update->skip_frame ? NULL : update->diff,
				GAMEBOY_WIDTH, GAMEBOY_HEIGHT, fb.pitch, PIXEL_MASK, display_palette);

        odroid_input_battery_level_read(&battery_state);

        xQueueReceive(vidQueue, &update, portMAX_DELAY);
    }


    // Draw hourglass
    odroid_display_lock();
    odroid_display_show_hourglass();
    odroid_display_unlock();


    videoTaskIsRunning = false;
    vTaskDelete(NULL);

    while (1) {}
}


volatile bool AudioTaskIsRunning = false;
void audioTask(void* arg)
{
  // sound
  uint16_t* param;

  AudioTaskIsRunning = true;
  while(1)
  {
    xQueuePeek(audioQueue, &param, portMAX_DELAY);

    if (param == 0)
    {
        // TODO: determine if this is still needed
        abort();
    }
    else if (param == 1)
    {
        break;
    }
    else
    {
        pcm_submit();
    }

    xQueueReceive(audioQueue, &param, portMAX_DELAY);
  }

  printf("audioTask: exiting.\n");
  odroid_audio_terminate();

  AudioTaskIsRunning = false;
  vTaskDelete(NULL);

  while (1) {}
}


static void SaveState()
{
    // Save sram
    odroid_input_battery_monitor_enabled_set(0);
    odroid_system_led_set(1);

    char* romPath = odroid_settings_RomFilePath_get();
    if (romPath)
    {
        char* fileName = odroid_util_GetFileName(romPath);
        if (!fileName) abort();

        char* pathName = odroid_sdcard_create_savefile_path(SD_BASE_PATH, fileName);
        if (!pathName) abort();

        FILE* f = fopen(pathName, "w");
        if (f == NULL)
        {
            printf("%s: fopen save failed\n", __func__);
            abort();
        }

        savestate(f);
        fclose(f);

        printf("%s: savestate OK.\n", __func__);

        free(pathName);
        free(fileName);
        free(romPath);
    }
    else
    {
        FILE* f = fopen(StateFileName, "w");
        if (f == NULL)
        {
            printf("SaveState: fopen save failed\n");
        }
        else
        {
            savestate(f);
            fclose(f);

            printf("SaveState: savestate OK.\n");
        }
    }


    odroid_system_led_set(0);
    odroid_input_battery_monitor_enabled_set(1);
}

static void LoadState(const char* cartName)
{
    char* romName = odroid_settings_RomFilePath_get();
    if (romName)
    {
        char* fileName = odroid_util_GetFileName(romName);
        if (!fileName) abort();

        char* pathName = odroid_sdcard_create_savefile_path(SD_BASE_PATH, fileName);
        if (!pathName) abort();

        FILE* f = fopen(pathName, "r");
        if (f == NULL)
        {
            printf("LoadState: fopen load failed\n");
        }
        else
        {
            loadstate(f);
            fclose(f);

            vram_dirty();
            pal_dirty();
            sound_dirty();
            mem_updatemap();

            printf("LoadState: loadstate OK.\n");
        }

        free(pathName);
        free(fileName);
        free(romName);
    }
    else
    {
        FILE* f = fopen(StateFileName, "r");
        if (f == NULL)
        {
            printf("LoadState: fopen load failed\n");
        }
        else
        {
            loadstate(f);
            fclose(f);

            vram_dirty();
            pal_dirty();
            sound_dirty();
            mem_updatemap();

            printf("LoadState: loadstate OK.\n");
        }
    }

	pal_set(odroid_settings_GBPalette_get());
    Volume = odroid_settings_Volume_get();
}

static void DoMenuHome()
{
    esp_err_t err;
    uint16_t* param = 1;

    // Clear audio to prevent studdering
    printf("PowerDown: stopping audio.\n");

    xQueueSend(audioQueue, &param, portMAX_DELAY);
    while (AudioTaskIsRunning) {}


    // Stop tasks
    printf("PowerDown: stopping tasks.\n");

    xQueueSend(vidQueue, &param, portMAX_DELAY);
    while (videoTaskIsRunning) {}


    // state
    printf("PowerDown: Saving state.\n");
    SaveState();


    // Set menu application
    odroid_system_application_set(0);


    // Reset
    esp_restart();
}
static void DoMenuHomeNoSave()
{
    esp_err_t err;
    uint16_t* param = 1;

    // Clear audio to prevent studdering
    printf("PowerDown: stopping audio.\n");

    xQueueSend(audioQueue, &param, portMAX_DELAY);
    while (AudioTaskIsRunning) {}


    // Stop tasks
    printf("PowerDown: stopping tasks.\n");

    xQueueSend(vidQueue, &param, portMAX_DELAY);
    while (videoTaskIsRunning) {}


    // Set menu application
    odroid_system_application_set(0);


    // Reset
    esp_restart();
}

void app_main(void)
{
    printf("gnuboy (%s-%s).\n", COMPILEDATE, GITREV);

    nvs_flash_init();

    odroid_system_init();

    odroid_input_gamepad_init();



    // Boot state overrides
    bool forceConsoleReset = false;

    switch (esp_sleep_get_wakeup_cause())
    {
        case ESP_SLEEP_WAKEUP_EXT0:
        {
            printf("app_main: ESP_SLEEP_WAKEUP_EXT0 deep sleep wake\n");
            break;
        }

        case ESP_SLEEP_WAKEUP_EXT1:
        case ESP_SLEEP_WAKEUP_TIMER:
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
        case ESP_SLEEP_WAKEUP_ULP:
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        {
            printf("app_main: Non deep sleep startup\n");

            odroid_gamepad_state bootState = odroid_input_read_raw();

            if (bootState.values[ODROID_INPUT_MENU])
            {
                // Force return to factory app to recover from
                // ROM loading crashes

                // Set menu application
                odroid_system_application_set(0);

                // Reset
                esp_restart();
            }

            if (bootState.values[ODROID_INPUT_START])
            {
                // Reset emulator if button held at startup to
                // override save state
                forceConsoleReset = true;
            }

            break;
        }
        default:
            printf("app_main: Not a deep sleep reset\n");
            break;
    }

    if (odroid_settings_StartAction_get() == ODROID_START_ACTION_RESTART)
    {
        forceConsoleReset = true;
        odroid_settings_StartAction_set(ODROID_START_ACTION_NORMAL);
    }


    // Display
    ili9341_prepare();
    ili9341_init();
    //odroid_display_show_splash();

    // Load ROM
    loader_init(NULL);
    

    // Clear display
    ili9341_blank_screen();

    // Audio hardware
    odroid_audio_init(odroid_settings_AudioSink_get(), AUDIO_SAMPLE_RATE);

    // Allocate display buffers
    displayBuffer[0] = heap_caps_malloc(GAMEBOY_WIDTH * GAMEBOY_HEIGHT, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    displayBuffer[1] = heap_caps_malloc(GAMEBOY_WIDTH * GAMEBOY_HEIGHT, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);

    if (displayBuffer[0] == 0 || displayBuffer[1] == 0)
        abort();

    framebuffer = displayBuffer[0];

    for (int i = 0; i < 2; ++i)
    {
        memset(displayBuffer[i], 0, GAMEBOY_WIDTH * GAMEBOY_HEIGHT);
    }

    printf("app_main: displayBuffer[0]=%p, [1]=%p\n", displayBuffer[0], displayBuffer[1]);

    // blue led
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 0);

    //  Charge
    odroid_input_battery_level_init();

    // video
    vidQueue = xQueueCreate(1, sizeof(uint16_t*));
    audioQueue = xQueueCreate(1, sizeof(uint16_t*));

    xTaskCreatePinnedToCore(&videoTask, "videoTask", 2048, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&audioTask, "audioTask", 2048, NULL, 5, NULL, 0); //768

    //debug_trace = 1;

    emu_reset();

    //&rtc.carry, &rtc.stop,
    rtc.d = 1;
    rtc.h = 1;
    rtc.m = 1;
    rtc.s = 1;
    rtc.t = 1;

    // vid_begin
    memset(&fb, 0, sizeof(fb));
    fb.w = 160;
  	fb.h = 144;
  	fb.pelsize = 1;
  	fb.pitch = fb.w * fb.pelsize;
  	fb.indexed = 0;
  	fb.ptr = framebuffer;
  	fb.enabled = 1;
  	fb.dirty = 1;


    // Note: Magic number obtained by adjusting until audio buffer overflows stop.
    const int audioBufferLength = AUDIO_SAMPLE_RATE / 10 + 1;
    //printf("CHECKPOINT AUDIO: HEAP:0x%x - allocating 0x%x\n", esp_get_free_heap_size(), audioBufferLength * sizeof(int16_t) * 2 * 2);
    const int AUDIO_BUFFER_SIZE = audioBufferLength * sizeof(int16_t) * 1;

    // pcm.len = count of 16bit samples (x2 for stereo)
    memset(&pcm, 0, sizeof(pcm));
    pcm.hz = AUDIO_SAMPLE_RATE;
  	pcm.stereo = 1;
  	pcm.len = /*pcm.hz / 2*/ audioBufferLength;
  	pcm.buf = heap_caps_malloc(AUDIO_BUFFER_SIZE, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
  	pcm.pos = 0;

    audioBuffer[0] = pcm.buf;
    audioBuffer[1] = heap_caps_malloc(AUDIO_BUFFER_SIZE, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);

    if (audioBuffer[0] == 0 || audioBuffer[1] == 0)
        abort();


    sound_reset();


    lcd_begin();


    // Load state
    LoadState(rom.name);

    //set initial scaling
    odroid_display_set_scale(GAMEBOY_WIDTH, GAMEBOY_HEIGHT, 1.2f);
    
    uint startTime;
    uint stopTime;
    uint totalElapsedTime = 0;
    uint actualFrameCount = 0;
    odroid_gamepad_state lastJoysticState, joystick;

    ushort menuButtonFrameCount = 0;
    bool ignoreMenuButton = lastJoysticState.values[ODROID_INPUT_MENU];

    // Reset if button held at startup
    if (forceConsoleReset)
    {
        emu_reset();
    }


    scaling_enabled = odroid_settings_ScaleDisabled_get(ODROID_SCALE_DISABLE_GB) ? false : true;

    odroid_input_gamepad_read(&lastJoysticState);
	char* statsbuf = calloc(1, 1024);

    while (true)
    {
		// Handle input
		{
			odroid_input_gamepad_read(&joystick);

			if (ignoreMenuButton)
			{
				ignoreMenuButton = lastJoysticState.values[ODROID_INPUT_MENU];
			}

			if (!ignoreMenuButton && lastJoysticState.values[ODROID_INPUT_MENU] && joystick.values[ODROID_INPUT_MENU])
			{
				++menuButtonFrameCount;
			}
			else
			{
				menuButtonFrameCount = 0;
			}

			//if (!lastJoysticState.Menu && joystick.Menu)
			if (menuButtonFrameCount > 60 * 1)
			{
				DoMenuHomeNoSave();

				gpio_set_level(GPIO_NUM_2, 0);
			}

			if (!ignoreMenuButton && lastJoysticState.values[ODROID_INPUT_MENU] && !joystick.values[ODROID_INPUT_MENU])
			{
				// Save state
				gpio_set_level(GPIO_NUM_2, 1);

				DoMenuHome();

				gpio_set_level(GPIO_NUM_2, 0);
			}


			if (!lastJoysticState.values[ODROID_INPUT_VOLUME] && joystick.values[ODROID_INPUT_VOLUME])
			{
				odroid_audio_volume_mute();
				printf("main: Volume=%d\n", odroid_audio_volume_get());
			}

			if (joystick.values[ODROID_INPUT_VOLUME] && !lastJoysticState.values[ODROID_INPUT_UP] && joystick.values[ODROID_INPUT_UP])
			{
				odroid_audio_volume_increase();
				printf("main: Volume=%d\n", odroid_audio_volume_get());
			}

			if (joystick.values[ODROID_INPUT_VOLUME] && !lastJoysticState.values[ODROID_INPUT_DOWN] && joystick.values[ODROID_INPUT_DOWN])
			{
				odroid_audio_volume_decrease();
				printf("main: Volume=%d\n", odroid_audio_volume_get());
			}

			// Scaling
			if (joystick.values[ODROID_INPUT_START] && !lastJoysticState.values[ODROID_INPUT_RIGHT] && joystick.values[ODROID_INPUT_RIGHT])
			{
				scaling_enabled = !scaling_enabled;
				odroid_settings_ScaleDisabled_set(ODROID_SCALE_DISABLE_GB, scaling_enabled ? 0 : 1);
			}

			// Cycle through palets
			if (joystick.values[ODROID_INPUT_SELECT] && !lastJoysticState.values[ODROID_INPUT_LEFT] && joystick.values[ODROID_INPUT_LEFT])
			{
				pal_next();
				odroid_settings_GBPalette_set(pal_get());
			}

			if (joystick.values[ODROID_INPUT_SELECT] && !lastJoysticState.values[ODROID_INPUT_RIGHT] && joystick.values[ODROID_INPUT_RIGHT])
			{
				pal_previous();
				odroid_settings_GBPalette_set(pal_get());
			}

			pad_set(PAD_UP, joystick.values[ODROID_INPUT_UP]);
			pad_set(PAD_RIGHT, joystick.values[ODROID_INPUT_RIGHT]);
			pad_set(PAD_DOWN, joystick.values[ODROID_INPUT_DOWN]);
			pad_set(PAD_LEFT, joystick.values[ODROID_INPUT_LEFT]);

			pad_set(PAD_SELECT, joystick.values[ODROID_INPUT_SELECT]);
			pad_set(PAD_START, joystick.values[ODROID_INPUT_START]);

			pad_set(PAD_A, joystick.values[ODROID_INPUT_A]);
			pad_set(PAD_B, joystick.values[ODROID_INPUT_B]);
		}

		// Emulate system for one frame
        startTime = xthal_get_ccount();
        run_to_vblank();
        stopTime = xthal_get_ccount();

        lastJoysticState = joystick;

		// Measure fps/speed
        if (stopTime > startTime) {
          elapsedTime = (stopTime - startTime);
		} else {
			// Handle overflown cycle counter
          elapsedTime = ((uint64_t)stopTime + (uint64_t)0xffffffff) - (startTime);
		}

        totalElapsedTime += elapsedTime;
        ++frame;

		// Cycle budget we can spend to emulate one frame to reach roughly 60 fps
		const int frameTime = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000 / 50;
		// Figure out if we should skip next frame
        skipFrame = (elapsedTime > frameTime);

		// if (skipFrame) {
		// 	printf("skipping frame because elapsedTime: %d\n", elapsedTime);
		// }

		// Display statistics every 60fps
        ++actualFrameCount;
        if (actualFrameCount == 60)
        {
          float seconds = totalElapsedTime / (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000.0f); // 240000000.0f; // (240Mhz)
          float fps = actualFrameCount / seconds;

		  // vTaskGetRunTimeStats(statsbuf);
		  // printf("%s\n", statsbuf);
		  
          printf("FPS:%f, BATTERY:%d [%d]\n", fps, battery_state.millivolts, battery_state.percentage);

          actualFrameCount = 0;
          totalElapsedTime = 0;
        }
    }
}
