// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "camera_index.h"
#include "Arduino.h"

#include "fb_gfx.h"
#include "fd_forward.h"
// #include "dl_lib.h"
#include "fr_forward.h"

#include "custom_CaptureWiFiClass.h"
#include "camera_driver_message.h"
#include "WiFiClient.h"

#define ENROLL_CONFIRM_TIMES 5
#define FACE_ID_SAVE_NUMBER 7

#define FACE_COLOR_WHITE 0x00FFFFFF
#define FACE_COLOR_BLACK 0x00000000
#define FACE_COLOR_RED 0x000000FF
#define FACE_COLOR_GREEN 0x0000FF00
#define FACE_COLOR_BLUE 0x00FF0000
#define FACE_COLOR_YELLOW (FACE_COLOR_RED | FACE_COLOR_GREEN)
#define FACE_COLOR_CYAN (FACE_COLOR_BLUE | FACE_COLOR_GREEN)
#define FACE_COLOR_PURPLE (FACE_COLOR_BLUE | FACE_COLOR_RED)

typedef struct
{
    size_t size;  //number of values used for filtering
    size_t index; //current value index
    size_t count; //value count
    int sum;
    int *values; //array to be filled with values
} ra_filter_t;

typedef struct
{
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static ra_filter_t ra_filter;
httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

static mtmn_config_t mtmn_config = {0};
static int8_t detection_enabled = 0;
static int8_t recognition_enabled = 0;
static int8_t is_enrolling = 0;
static face_id_list id_list = {0};

CustomCaptureWiFiClass captureWiFi;

static ra_filter_t *ra_filter_init(ra_filter_t *filter, size_t sample_size)
{
    memset(filter, 0, sizeof(ra_filter_t));

    filter->values = (int *)malloc(sample_size * sizeof(int));
    if (!filter->values)
    {
        return NULL;
    }
    memset(filter->values, 0, sample_size * sizeof(int));

    filter->size = sample_size;
    return filter;
}

static int ra_filter_run(ra_filter_t *filter, int value)
{
    if (!filter->values)
    {
        return value;
    }
    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += filter->values[filter->index];
    filter->index++;
    filter->index = filter->index % filter->size;
    if (filter->count < filter->size)
    {
        filter->count++;
    }
    return filter->sum / filter->count;
}

static void rgb_print(dl_matrix3du_t *image_matrix, uint32_t color, const char *str)
{
    fb_data_t fb;
    fb.width = image_matrix->w;
    fb.height = image_matrix->h;
    fb.data = image_matrix->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;
    fb_gfx_print(&fb, (fb.width - (strlen(str) * 14)) / 2, 10, color, str);
}

static int rgb_printf(dl_matrix3du_t *image_matrix, uint32_t color, const char *format, ...)
{
    char loc_buf[64];
    char *temp = loc_buf;
    int len;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    len = vsnprintf(loc_buf, sizeof(loc_buf), format, arg);
    va_end(copy);
    if (len >= sizeof(loc_buf))
    {
        temp = (char *)malloc(len + 1);
        if (temp == NULL)
        {
            return 0;
        }
    }
    vsnprintf(temp, len + 1, format, arg);
    va_end(arg);
    rgb_print(image_matrix, color, temp);
    if (len > 64)
    {
        free(temp);
    }
    return len;
}

static void draw_face_boxes(dl_matrix3du_t *image_matrix, box_array_t *boxes, int face_id)
{
    int x, y, w, h, i;
    uint32_t color = FACE_COLOR_YELLOW;
    if (face_id < 0)
    {
        color = FACE_COLOR_RED;
    }
    else if (face_id > 0)
    {
        color = FACE_COLOR_GREEN;
    }
    fb_data_t fb;
    fb.width = image_matrix->w;
    fb.height = image_matrix->h;
    fb.data = image_matrix->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;
    for (i = 0; i < boxes->len; i++)
    {
        // rectangle box
        x = (int)boxes->box[i].box_p[0];
        y = (int)boxes->box[i].box_p[1];
        w = (int)boxes->box[i].box_p[2] - x + 1;
        h = (int)boxes->box[i].box_p[3] - y + 1;
        fb_gfx_drawFastHLine(&fb, x, y, w, color);
        fb_gfx_drawFastHLine(&fb, x, y + h - 1, w, color);
        fb_gfx_drawFastVLine(&fb, x, y, h, color);
        fb_gfx_drawFastVLine(&fb, x + w - 1, y, h, color);
#if 0
        // landmark
        int x0, y0, j;
        for (j = 0; j < 10; j+=2) {
            x0 = (int)boxes->landmark[i].landmark_p[j];
            y0 = (int)boxes->landmark[i].landmark_p[j+1];
            fb_gfx_fillRect(&fb, x0, y0, 3, 3, color);
        }
#endif
    }
}

static int run_face_recognition(dl_matrix3du_t *image_matrix, box_array_t *net_boxes)
{
    dl_matrix3du_t *aligned_face = NULL;
    int matched_id = 0;

    aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);
    if (!aligned_face)
    {
        Serial.println("Could not allocate face recognition buffer");
        return matched_id;
    }
    if (align_face(net_boxes, image_matrix, aligned_face) == ESP_OK)
    {
        if (is_enrolling == 1)
        {
            int8_t left_sample_face = enroll_face(&id_list, aligned_face);

            if (left_sample_face == (ENROLL_CONFIRM_TIMES - 1))
            {
                Serial.printf("Enrolling Face ID: %d\n", id_list.tail);
            }
            Serial.printf("Enrolling Face ID: %d sample %d\n", id_list.tail, ENROLL_CONFIRM_TIMES - left_sample_face);
            rgb_printf(image_matrix, FACE_COLOR_CYAN, "ID[%u] Sample[%u]", id_list.tail, ENROLL_CONFIRM_TIMES - left_sample_face);
            if (left_sample_face == 0)
            {
                is_enrolling = 0;
                Serial.printf("Enrolled Face ID: %d\n", id_list.tail);
            }
        }
        else
        {
            matched_id = recognize_face(&id_list, aligned_face);
            if (matched_id >= 0)
            {
                Serial.printf("Match Face ID: %u\n", matched_id);
                rgb_printf(image_matrix, FACE_COLOR_GREEN, "Hello Subject %u", matched_id);
            }
            else
            {
                Serial.println("No Match Found");
                rgb_print(image_matrix, FACE_COLOR_RED, "Intruder Alert!");
                matched_id = -1;
            }
        }
    }
    else
    {
        Serial.println("Face Not Aligned");
        //rgb_print(image_matrix, FACE_COLOR_YELLOW, "Human Detected");
    }

    dl_matrix3du_free(aligned_face);
    return matched_id;
}

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len)
{
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if (!index)
    {
        j->len = 0;
    }
    if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK)
    {
        return 0;
    }
    j->len += len;
    return len;
}

static esp_err_t capture_handler()
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Camera capture failed");
        // httpd_resp_send_500(req);
        captureWiFi.writeToCameraDriver(NULL, camera_driver_msg_type_t::NO_TYPE, 0);
        return ESP_FAIL;
    }

    /*
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    */

    size_t out_len, out_width, out_height;
    uint8_t *out_buf;
    bool s;
    bool detected = false;
    int face_id = 0;
    if (!detection_enabled || fb->width > 400)
    {
        size_t fb_len = 0;
        if (fb->format == PIXFORMAT_JPEG)
        {
            fb_len = fb->len;
            LOGV("Trying to send image capture, with size: %u\n", fb->len);
            // res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
            res = (esp_err_t)captureWiFi.writeToCameraDriver((const char *)fb->buf, camera_driver_msg_type_t::CAPTURE, fb->len);
        }
        else
        {
            /* 
             *  SEE WHAT THIS DOES TODO
            jpg_chunking_t jchunk = {req, 0};
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
            res = (esp_err_t) captureWiFi.writeToCameraDriver(NULL, camera_driver_msg_type_t::CAPTURE, 0);
            fb_len = jchunk.len; */
        }
        esp_camera_fb_return(fb);
        int64_t fr_end = esp_timer_get_time();
        Serial.printf("JPG: %uB %ums\n", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start) / 1000));
        return res;
    }

    dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
    if (!image_matrix)
    {
        esp_camera_fb_return(fb);
        Serial.println("dl_matrix3du_alloc failed");
        // httpd_resp_send_500(req);
        captureWiFi.writeToCameraDriver(NULL, camera_driver_msg_type_t::NO_TYPE, 0);
        return ESP_FAIL;
    }

    out_buf = image_matrix->item;
    out_len = fb->width * fb->height * 3;
    out_width = fb->width;
    out_height = fb->height;

    s = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
    esp_camera_fb_return(fb);
    if (!s)
    {
        dl_matrix3du_free(image_matrix);
        Serial.println("to rgb888 failed");
        // httpd_resp_send_500(req);
        captureWiFi.writeToCameraDriver(NULL, camera_driver_msg_type_t::NO_TYPE, 0);
        return ESP_FAIL;
    }

    box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);

    if (net_boxes)
    {
        detected = true;
        if (recognition_enabled)
        {
            face_id = run_face_recognition(image_matrix, net_boxes);
        }
        draw_face_boxes(image_matrix, net_boxes, face_id);
        free(net_boxes->box);
        free(net_boxes->landmark);
        free(net_boxes);
    }

    /*
     * SEE WHAT THIS DOES TODO
    jpg_chunking_t jchunk = {req, 0};
    s = fmt2jpg_cb(out_buf, out_len, out_width, out_height, PIXFORMAT_RGB888, 90, jpg_encode_stream, &jchunk);
    dl_matrix3du_free(image_matrix);
    if(!s){
        Serial.println("JPEG compression failed");
        return ESP_FAIL;
    }

    int64_t fr_end = esp_timer_get_time();
    Serial.printf("FACE: %uB %ums %s%d\n", (uint32_t)(jchunk.len), (uint32_t)((fr_end - fr_start)/1000), detected?"DETECTED ":"", face_id);
    */
    return res;
}

void *stream_handler(void *varg)
{
    LOGV("Inside\n\n\n");
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char part_buf[64];
    dl_matrix3du_t *image_matrix = NULL;
    bool detected = false;
    int face_id = 0;
    int64_t fr_start = 0;
    int64_t fr_ready = 0;
    int64_t fr_face = 0;
    int64_t fr_recognize = 0;
    int64_t fr_encode = 0;

    static int64_t last_frame = 0;
    if (!last_frame)
    {
        last_frame = esp_timer_get_time();
    }

    // LOGV("checkpoint\n");
    WiFiClient stream_conn = WiFiClient();
    // LOGV("checkpoint\n");
    stream_conn.connect(captureWiFi.driver_connection.remoteIP(), captureWiFi.driver_connection.remotePort());
    // LOGV("checkpoint\n");

    while (true)
    {
        // LOGV("checkpoint d\n");

        if (stream_conn.available())
        {
            // LOGV("checkpoint stream_conn.available\n");

            capture_camera_driver_message_t control_msg = capture_camera_driver_message_t();
            control_msg.read_metadata_from_driver(stream_conn, captureWiFi.capture);
            if (control_msg.type == camera_driver_msg_type_t::STREAM_STOP)
            {
                LOGV("checkpoint stream_stop\n");
                pthread_exit(NULL);
            }
        }

        // TODO: How frames are sent?!
        detected = false;
        face_id = 0;
        fb = esp_camera_fb_get();
        // LOGV("checkpoint e\n");
        if (!fb)
        {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        }
        else
        {
            // LOGV("checkpoint e\n");

            fr_start = esp_timer_get_time();
            fr_ready = fr_start;
            fr_face = fr_start;
            fr_encode = fr_start;
            fr_recognize = fr_start;
            if (!detection_enabled || fb->width > 400)
            {
                // LOGV("checkpoint !detection_enabled || fb->width > 400\n");

                if (fb->format != PIXFORMAT_JPEG)
                {
                    // LOGV("checkpoint fb->format != PIXFORMAT_JPEG\n");

                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if (!jpeg_converted)
                    {
                        Serial.println("JPEG compression failed");
                        res = ESP_FAIL;
                    }
                }
                else
                {
                    // LOGV("checkpoint ea\n");

                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
            }
            else
            {
                // LOGV("checkpoint f\n");

                image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);

                if (!image_matrix)
                {
                    // LOGV("checkpoint fa\n");
                    Serial.println("dl_matrix3du_alloc failed");
                    res = ESP_FAIL;
                }
                else
                {
                    // LOGV("checkpoint fd\n");
                    if (!fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item))
                    {
                        // LOGV("checkpoint fgh\n");
                        Serial.println("fmt2rgb888 failed");
                        res = ESP_FAIL;
                    }
                    else
                    {
                        // LOGV("checkpoint fss\n");

                        fr_ready = esp_timer_get_time();
                        box_array_t *net_boxes = NULL;
                        if (detection_enabled)
                        {
                            // LOGV("checkpoint fssaf\n");

                            net_boxes = face_detect(image_matrix, &mtmn_config);
                        }
                        fr_face = esp_timer_get_time();
                        fr_recognize = fr_face;
                        if (net_boxes || fb->format != PIXFORMAT_JPEG)
                        {
                            // LOGV("checkpoint fsfadfa\n");

                            if (net_boxes)
                            {
                                // LOGV("checkpoint scvvfedsf\n");

                                detected = true;
                                if (recognition_enabled)
                                {
                                    // LOGV("checkpoint fsfac sdf\n");

                                    face_id = run_face_recognition(image_matrix, net_boxes);
                                }
                                fr_recognize = esp_timer_get_time();
                                draw_face_boxes(image_matrix, net_boxes, face_id);
                                free(net_boxes->box);
                                free(net_boxes->landmark);
                                free(net_boxes);
                            }
                            if (!fmt2jpg(image_matrix->item, fb->width * fb->height * 3, fb->width, fb->height, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len))
                            {
                                // LOGV("checkpoint fsdscde33\n");

                                Serial.println("fmt2jpg failed");
                                res = ESP_FAIL;
                            }
                            esp_camera_fb_return(fb);
                            fb = NULL;
                        }
                        else
                        {
                            // LOGV("checkpoint fcv2355\n");

                            _jpg_buf = fb->buf;
                            _jpg_buf_len = fb->len;
                        }
                        fr_encode = esp_timer_get_time();
                    }
                    dl_matrix3du_free(image_matrix);
                }
            }
        }
        capture_camera_driver_message_t tmp = capture_camera_driver_message_t();

        if (res == ESP_OK)
        {
            // LOGV("checkpoint x, _jpg_buf_len: %u\n", _jpg_buf_len);

            memset(part_buf, 0, 64);
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            // LOGV("hlen is: %u\n", hlen);
            // LOGV("part_buf is: %s\n", part_buf);
            captureWiFi.readFromCameraDriver(&tmp, stream_conn);
            res = (esp_err_t)captureWiFi.writeToCameraDriver((const char *)part_buf, camera_driver_msg_type_t::STREAM_CONTENT_1, hlen, stream_conn);
            // res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        // LOGV("checkpoint x done\n");

        if (res == ESP_OK)
        {
            // LOGV("checkpoint y\n");
            captureWiFi.readFromCameraDriver(&tmp, stream_conn);
            res = (esp_err_t)captureWiFi.writeToCameraDriver((const char *)_jpg_buf, camera_driver_msg_type_t::STREAM_CONTENT_2, _jpg_buf_len, stream_conn);
            // res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        // LOGV("checkpoint y done\n");

        if (res == ESP_OK)
        {
            // LOGV("checkpoint z\n");
            captureWiFi.readFromCameraDriver(&tmp, stream_conn);
            res = (esp_err_t)captureWiFi.writeToCameraDriver((const char *)_STREAM_BOUNDARY, camera_driver_msg_type_t::STREAM_CONTENT_3, strlen(_STREAM_BOUNDARY), stream_conn);
            // res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        // LOGV("checkpoint z done\n");

        if (fb)
        {
            // LOGV("checkpoint fb return\n");

            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        }
        else if (_jpg_buf)
        {
            // LOGV("checkpoint jpg_buf\n");

            free(_jpg_buf);
            _jpg_buf = NULL;
        }

        // LOGV("checkpoint outside\n");

        if (res != ESP_OK)
        {
            // LOGV("checkpoint afterwards\n");

            break;
        }
        int64_t fr_end = esp_timer_get_time();

        int64_t ready_time = (fr_ready - fr_start) / 1000;
        int64_t face_time = (fr_face - fr_ready) / 1000;
        int64_t recognize_time = (fr_recognize - fr_face) / 1000;
        int64_t encode_time = (fr_encode - fr_recognize) / 1000;
        int64_t process_time = (fr_encode - fr_start) / 1000;

        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        LOGV("checkpoint about to call ra_filter_run\n");

        // uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
        // LOGV("checkpoint about to print\n");
        // SPECIAL_LOGV("Avg frame time: %u (%.1ffps)\n", avg_frame_time, 1000.0/avg_frame_time);

        // Serial.printf("MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps)\n",
        //               (uint32_t)(_jpg_buf_len),
        //               (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
        //               avg_frame_time, 1000.0 / avg_frame_time);

        // Serial.printf("MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps), %u+%u+%u+%u=%u %s%d\n",
        //               (uint32_t)(_jpg_buf_len),
        //               (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
        //               avg_frame_time, 1000.0 / avg_frame_time,
        //               (uint32_t)ready_time, (uint32_t)face_time, (uint32_t)recognize_time, (uint32_t)encode_time, (uint32_t)process_time,
        //               (detected) ? "DETECTED " : "", face_id);
    }

    last_frame = 0;
    return ESP_OK;
}

static esp_err_t cmd_handler(capture_camera_driver_message_t &request)
{
    char *buf;
    size_t buf_len;
    char variable[32] = {
        0,
    };
    char value[32] = {
        0,
    };

    buf_len = request.len;
    LOGV("buf_len: %u\n", buf_len);
    if (buf_len > 1)
    {
        buf = (char *)malloc(buf_len);
        if (!buf)
        {
            // httpd_resp_send_500(req);
            captureWiFi.writeToCameraDriver(NULL, camera_driver_msg_type_t::NO_TYPE, 0);
            return ESP_FAIL;
        }

        LOGV("Start memcpy\n");
        memcpy(variable, request.payload, 32);
        memcpy(value, &request.payload[32], 32);
        LOGV("Finish memcpy\n");
    }
    else
    {
        // httpd_resp_send_404(req);
        LOGV("Fail\n");
        captureWiFi.writeToCameraDriver(NULL, camera_driver_msg_type_t::NO_TYPE, 0);
        return ESP_FAIL;
    }
    LOGV("Start now\n");

    int val = atoi(value);
    LOGV("value after atoi(): %d\n", val);

    sensor_t *s = esp_camera_sensor_get();
    int res = 0;

    LOGV("variable is: %s\n", variable);

    if (!strcmp(variable, "framesize"))
    {
        LOGV("strvmp\n");

        if (s->pixformat == PIXFORMAT_JPEG)
        {
            res = s->set_framesize(s, (framesize_t)val);
            LOGV("after strvmp\n");
        }
        LOGV("Finish framesize\n");
    }
    else if (!strcmp(variable, "quality"))
        res = s->set_quality(s, val);
    else if (!strcmp(variable, "contrast"))
        res = s->set_contrast(s, val);
    else if (!strcmp(variable, "brightness"))
        res = s->set_brightness(s, val);
    else if (!strcmp(variable, "saturation"))
        res = s->set_saturation(s, val);
    else if (!strcmp(variable, "gainceiling"))
        res = s->set_gainceiling(s, (gainceiling_t)val);
    else if (!strcmp(variable, "colorbar"))
        res = s->set_colorbar(s, val);
    else if (!strcmp(variable, "awb"))
        res = s->set_whitebal(s, val);
    else if (!strcmp(variable, "agc"))
        res = s->set_gain_ctrl(s, val);
    else if (!strcmp(variable, "aec"))
        res = s->set_exposure_ctrl(s, val);
    else if (!strcmp(variable, "hmirror"))
        res = s->set_hmirror(s, val);
    else if (!strcmp(variable, "vflip"))
        res = s->set_vflip(s, val);
    else if (!strcmp(variable, "awb_gain"))
        res = s->set_awb_gain(s, val);
    else if (!strcmp(variable, "agc_gain"))
        res = s->set_agc_gain(s, val);
    else if (!strcmp(variable, "aec_value"))
        res = s->set_aec_value(s, val);
    else if (!strcmp(variable, "aec2"))
        res = s->set_aec2(s, val);
    else if (!strcmp(variable, "dcw"))
        res = s->set_dcw(s, val);
    else if (!strcmp(variable, "bpc"))
        res = s->set_bpc(s, val);
    else if (!strcmp(variable, "wpc"))
        res = s->set_wpc(s, val);
    else if (!strcmp(variable, "raw_gma"))
        res = s->set_raw_gma(s, val);
    else if (!strcmp(variable, "lenc"))
        res = s->set_lenc(s, val);
    else if (!strcmp(variable, "special_effect"))
        res = s->set_special_effect(s, val);
    else if (!strcmp(variable, "wb_mode"))
        res = s->set_wb_mode(s, val);
    else if (!strcmp(variable, "ae_level"))
        res = s->set_ae_level(s, val);
    else if (!strcmp(variable, "face_detect"))
    {
        detection_enabled = val;
        if (!detection_enabled)
        {
            recognition_enabled = 0;
        }
    }
    else if (!strcmp(variable, "face_enroll"))
        is_enrolling = val;
    else if (!strcmp(variable, "face_recognize"))
    {
        recognition_enabled = val;
        if (recognition_enabled)
        {
            detection_enabled = val;
        }
    }
    else
    {
        res = -1;
    }

    if (res)
    {
        LOGV("operation fail\n");
        captureWiFi.writeToCameraDriver(NULL, camera_driver_msg_type_t::NO_TYPE, 0);
        return ESP_FAIL;
        // return httpd_resp_send_500(req);
    }

    LOGV("cmd handler returns\n");

    return 0;
    // httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    // return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler()
{
    static char json_response[1024];

    sensor_t *s = esp_camera_sensor_get();
    char *p = json_response;
    *p++ = '{';

    p += sprintf(p, "\"framesize\":%u,", s->status.framesize);
    p += sprintf(p, "\"quality\":%u,", s->status.quality);
    p += sprintf(p, "\"brightness\":%d,", s->status.brightness);
    p += sprintf(p, "\"contrast\":%d,", s->status.contrast);
    p += sprintf(p, "\"saturation\":%d,", s->status.saturation);
    p += sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
    p += sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
    p += sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
    p += sprintf(p, "\"awb\":%u,", s->status.awb);
    p += sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
    p += sprintf(p, "\"aec\":%u,", s->status.aec);
    p += sprintf(p, "\"aec2\":%u,", s->status.aec2);
    p += sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
    p += sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
    p += sprintf(p, "\"agc\":%u,", s->status.agc);
    p += sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
    p += sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
    p += sprintf(p, "\"bpc\":%u,", s->status.bpc);
    p += sprintf(p, "\"wpc\":%u,", s->status.wpc);
    p += sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
    p += sprintf(p, "\"lenc\":%u,", s->status.lenc);
    p += sprintf(p, "\"vflip\":%u,", s->status.vflip);
    p += sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
    p += sprintf(p, "\"dcw\":%u,", s->status.dcw);
    p += sprintf(p, "\"colorbar\":%u,", s->status.colorbar);
    p += sprintf(p, "\"face_detect\":%u,", detection_enabled);
    p += sprintf(p, "\"face_enroll\":%u,", is_enrolling);
    p += sprintf(p, "\"face_recognize\":%u", recognition_enabled);
    *p++ = '}';
    *p++ = 0;
    /*
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_response, strlen(json_response));
    */
    captureWiFi.writeToCameraDriver(json_response, camera_driver_msg_type_t::STATUS, strlen(json_response));
    return 0;
}

static esp_err_t index_handler()
{
    sensor_t *s = esp_camera_sensor_get();
    char buf[] = "ov2640";
    if (s->id.PID == OV3660_PID)
    {
        memcpy(buf, "ov3660", strlen(buf));
    }
    captureWiFi.writeToCameraDriver(buf, camera_driver_msg_type_t::INDEX, strlen(buf));
    return 0;
}

void startCameraServer(CustomCaptureWiFiClass cWiFi)
{
    captureWiFi = cWiFi;

    ra_filter_init(&ra_filter, 20);

    mtmn_config.min_face = 80;
    mtmn_config.pyramid = 0.7;
    mtmn_config.p_threshold.score = 0.6;
    mtmn_config.p_threshold.nms = 0.7;
    mtmn_config.r_threshold.score = 0.7;
    mtmn_config.r_threshold.nms = 0.7;
    mtmn_config.r_threshold.candidate_number = 4;
    mtmn_config.o_threshold.score = 0.7;
    mtmn_config.o_threshold.nms = 0.4;
    mtmn_config.o_threshold.candidate_number = 1;

    face_id_init(&id_list, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);

    // Busy wait on the device side to handle requests from the driver
    LOGV("Camera ready for new connection...\n");

    while (true)
    {
        if (captureWiFi.driver_connection.available())
        {
            LOGV("New available message...\n");
            capture_camera_driver_message_t request = capture_camera_driver_message_t();
            captureWiFi.readFromCameraDriver(&request);

            LOGV("Processing new request...\n");

            pthread_t tid = 1;

            switch (request.type)
            {
            case camera_driver_msg_type_t::INDEX:
                index_handler();
                break;
            case camera_driver_msg_type_t::CONTROL:
                cmd_handler(request);
                LOGV("after cmd handler returns\n");
                break;
            case camera_driver_msg_type_t::STATUS:
                status_handler();
                break;
            case camera_driver_msg_type_t::CAPTURE:
                capture_handler();
                LOGV("Finish processing capture_handler\n");
                break;
            case camera_driver_msg_type_t::STREAM_START:
                LOGV("Receive stream start cmd!!!\n");
                pthread_create(&tid, NULL, stream_handler, NULL);
                break;
            default:
                Serial.println("Unexpected message type!");
                break;
            }
        }
    }
}
