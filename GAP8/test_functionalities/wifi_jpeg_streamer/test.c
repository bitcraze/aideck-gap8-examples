#include "pmsis.h"
#include "bsp/camera/himax.h"
#include "bsp/camera/mt9v034.h"
#include "bsp/transport/nina_w10.h"
#include "tools/frame_streamer.h"
#include "stdio.h"

#if defined(CONFIG_GAPUINO) || defined(CONFIG_AI_DECK)
#define CAM_WIDTH    324
#define CAM_HEIGHT   244
#else
#define CAM_WIDTH    320
#define CAM_HEIGHT   240
#endif

static pi_task_t task1;
static pi_task_t task2;
static unsigned char *imgBuff0;
static unsigned char *imgBuff1;
static struct pi_device camera;
static struct pi_device wifi;
static frame_streamer_t *streamer1;
static frame_streamer_t *streamer2;
static pi_buffer_t buffer;
static pi_buffer_t buffer2;
static volatile int stream1_done;
static volatile int stream2_done;

static void streamer_handler(void *arg);

unsigned int perf[2];


static void cam_handler(void *arg)
{
#ifdef PERF
    pi_perf_stop();
    perf[0] = pi_perf_read(PI_PERF_CYCLES);
    pi_perf_start();
#endif

    pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);

    stream1_done = 0;
    stream2_done = 0;

    frame_streamer_send_async(streamer1, &buffer, pi_task_callback(&task1, streamer_handler, (void *)&stream1_done));

    return;

#if 0
    for (int i=0; i<CAM_HEIGHT; i++)
    {
        for (int j=0; j<CAM_WIDTH; j++)
        {
            int index = i*CAM_WIDTH + j;
            if (imgBuff0[index] > 64)
                imgBuff1[index] = imgBuff0[index];
            else
                imgBuff1[index] = 0;
        }
    }

    frame_streamer_send_async(streamer2, &buffer2, pi_task_callback(&task2, streamer_handler, (void *)&stream2_done));
#endif
}



static void streamer_handler(void *arg)
{
    *(int *)arg = 1;
    if (stream1_done) // && stream2_done)
    {
#ifdef PERF
        pi_perf_stop();
        perf[1] = pi_perf_read(PI_PERF_CYCLES);
        printf ("%d cycles\n", (perf[1] - perf[0]));
        pi_perf_start();
#endif

        pi_camera_capture_async(&camera, imgBuff0, CAM_WIDTH*CAM_HEIGHT, pi_task_callback(&task1, cam_handler, NULL));
        pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
    }
}



static int open_pi_camera_himax(struct pi_device *device)
{
    struct pi_himax_conf cam_conf;

    pi_himax_conf_init(&cam_conf);

    cam_conf.format = PI_CAMERA_QQVGA;

    pi_open_from_conf(device, &cam_conf);
    if (pi_camera_open(device))
        return -1;

    return 0;
}



static int open_pi_camera_mt9v034(struct pi_device *device)
{
    struct pi_mt9v034_conf cam_conf;

    pi_mt9v034_conf_init(&cam_conf);

    cam_conf.format = PI_CAMERA_QVGA;

    pi_open_from_conf(device, &cam_conf);
    if (pi_camera_open(device))
        return -1;

    return 0;
}



static int open_camera(struct pi_device *device)
{
#ifdef CONFIG_GAPOC_A
    return open_pi_camera_mt9v034(device);
#endif
#if defined(CONFIG_GAPUINO) || defined(CONFIG_AI_DECK)
    return open_pi_camera_himax(device);
#endif
    return -1;
}


static int open_wifi(struct pi_device *device)
{
    struct pi_nina_w10_conf nina_conf;

    pi_nina_w10_conf_init(&nina_conf);

    nina_conf.ssid = "";
    nina_conf.passwd = "";
    nina_conf.ip_addr = "0.0.0.0";
    nina_conf.port = 5555;
    pi_open_from_conf(device, &nina_conf);
    if (pi_transport_open(device))
        return -1;

    return 0;
}
    
static frame_streamer_t *open_streamer(char *name)
{
    struct frame_streamer_conf frame_streamer_conf;

    frame_streamer_conf_init(&frame_streamer_conf);

    frame_streamer_conf.transport = &wifi;
    frame_streamer_conf.format = FRAME_STREAMER_FORMAT_JPEG;
    frame_streamer_conf.width = CAM_WIDTH;
    frame_streamer_conf.height = CAM_HEIGHT;
    frame_streamer_conf.depth = 1;
    frame_streamer_conf.name = name;

    return frame_streamer_open(&frame_streamer_conf);
}
static pi_task_t led_task;
static int led_val = 0;
static struct pi_device gpio_device;
static void led_handle(void *arg)
{
    pi_gpio_pin_write(&gpio_device, 2, led_val);
    led_val ^= 1;
    pi_task_push_delayed_us(pi_task_callback(&led_task, led_handle, NULL), 500000);
}

int wifi_streamer()
{
    printf("Entering main controller...\n");
    pi_perf_conf(1 << PI_PERF_CYCLES );
    pi_perf_start();

    //pi_time_wait_us(1*1000*1000);

    pi_freq_set(PI_FREQ_DOMAIN_FC, 150000000);

    //    pi_gpio_pin_configure(&gpio_device, 2, PI_GPIO_OUTPUT);

    // pi_task_push_delayed_us(pi_task_callback(&led_task, led_handle, NULL), 500000);

    imgBuff0 = (unsigned char *)pmsis_l2_malloc((CAM_WIDTH*CAM_HEIGHT)*sizeof(unsigned char));
    if (imgBuff0 == NULL) {
        printf("Failed to allocate Memory for Image \n");
        goto error;
    }

    imgBuff1 = (unsigned char *)pmsis_l2_malloc((CAM_WIDTH*CAM_HEIGHT)*sizeof(unsigned char));
    if (imgBuff1 == NULL) {
        printf("Failed to allocate Memory for Image \n");
        goto error;
    }

    if (open_camera(&camera))
    {
        printf("Failed to open camera\n");
        goto error;
    }
    printf("0\n");

    if (open_wifi(&wifi))
    {
        printf("Failed to open wifi\n");
        goto error;
    }

    streamer1 = open_streamer("camera");
    if (streamer1 == NULL)
        goto error;

    //pi_freq_set(PI_FREQ_DOMAIN_CL, 40000000);

    //streamer2 = open_streamer("filtered");
    //if (streamer2 == NULL)
    //  return -1;

    pi_buffer_init(&buffer, PI_BUFFER_TYPE_L2, imgBuff0);
    pi_buffer_set_format(&buffer, CAM_WIDTH, CAM_HEIGHT, 1, PI_BUFFER_FORMAT_GRAY);

    pi_buffer_init(&buffer2, PI_BUFFER_TYPE_L2, imgBuff1);
    pi_buffer_set_format(&buffer2, CAM_WIDTH, CAM_HEIGHT, 1, PI_BUFFER_FORMAT_GRAY);

    pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
    pi_camera_capture_async(&camera, imgBuff0, CAM_WIDTH*CAM_HEIGHT, pi_task_callback(&task1, cam_handler, NULL));
    pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);

    while(1)
    {
        pi_yield();
    }

    pmsis_exit(0);

error:
    pmsis_exit(-1);

}

/* Program Entry. */
int main(void)
{
    printf("\n\n\t *** Wifi Streamer ***\n\n");
    return pmsis_kickoff((void *) wifi_streamer);
}
