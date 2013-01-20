/*
 * Lerna
 *
 * Copyright (c) 2013 Jesus Ojeda
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by 
 * this license (the "Software") to use, reproduce, display, distribute, 
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer, must
 * be included in all copies of the Software, in whole or in part, and all
 * derivative works of the Software, unless such copies or derivative works are
 * solely in the form of machine-executable object code generated by a source
 * language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
*/

#include "lerna.h"
#include <hidapi/hidapi.h>
#include <tinycthread/tinycthread.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define INT16_FLOAT_MM  1e-3f
#define INT16_FLOAT_M11 3.0517578125e-5f // 1.f/32768.f
#define UINT8_FLOAT_01  3.9215686274e-3f // 1.f/255.f

#define HYDRA_VENDORID 0x1532
#define HYDRA_PRODUCTID 0x0300

#define LERNA_SENDING 0x1
#define LERNA_SPHTRAC 0x2
#define LERNA_MAX_HISTORY 65 //Intended MAX + 1
#define LERNA_EPSILON 1e-4f  //Max jump for hemisphere tracking

typedef unsigned char ubyte;
typedef short int16;

const ubyte HYDRA_FEATURE_REPORT[] = {
  0x00, // first byte must be report type
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x06, 0x00
};
const int HYDRA_FEATURE_REPORT_LEN = 91;
const ubyte HYDRA_GAMEPAD_COMMAND[] = {
  0x00, // first byte must be report type
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x05, 0x00
};
const int HYDRA_GAMEPAD_COMMAND_LEN = 91;
const ubyte CD_offset[2] = {8, 30}; //inital offset for each controller_data in buffer

struct _lerna_internaldata {
  ubyte _runTh;
  ubyte _status;
  thrd_t _th;
  hid_device *_hydra_ctl;
  hid_device *_hydra_dat;
  time_t _time;
} _lid;

struct _lerna_internal_controller_data {
  float _hemi_mirror[2];
  lernaDualControllerData _history[256];
  ubyte _prev, _last;
} _lcd;

void _hemisphere_tracking(int which_cont) {
  float old[3] = {_lcd._history[_lcd._prev].data[which_cont].pos[0],
    _lcd._history[_lcd._prev].data[which_cont].pos[1],
    _lcd._history[_lcd._prev].data[which_cont].pos[2]};
  float report[3] = {_lcd._history[_lcd._last].data[which_cont].pos[0],
    _lcd._history[_lcd._last].data[which_cont].pos[1],
    _lcd._history[_lcd._last].data[which_cont].pos[2]};
  float negat[3] = {-_lcd._history[_lcd._last].data[which_cont].pos[0],
    -_lcd._history[_lcd._last].data[which_cont].pos[1],
    -_lcd._history[_lcd._last].data[which_cont].pos[2]};

  float repo = (report[0] - old[0]) * (report[0] - old[0]) +
    (report[1] - old[1]) * (report[1] - old[1]) +
    (report[2] - old[2]) * (report[2] - old[2]);
  float nega = (negat[0] - old[0]) * (negat[0] - old[0]) +
    (negat[1] - old[1]) * (negat[1] - old[1]) +
    (negat[2] - old[2]) * (negat[2] - old[2]);

  if(repo - nega > LERNA_EPSILON) {
    _lcd._hemi_mirror[which_cont] *= -1.f;
    _lcd._history[_lcd._last].data[which_cont].pos[0] *= -1.f;
    _lcd._history[_lcd._last].data[which_cont].pos[1] *= -1.f;
    _lcd._history[_lcd._last].data[which_cont].pos[2] *= -1.f;
  }
}

float _qtmp[4] = {0.f};
void _quat_180Xrot(float v[4]) {
  _qtmp[0] = -v[1];
  _qtmp[1] = v[0];
  _qtmp[2] = v[3];
  _qtmp[3] = -v[2];
  memcpy(v, &_qtmp, sizeof(float)*4);
}

void _processData(ubyte buf[]) {
  lernaDualControllerData *_new = &_lcd._history[buf[7]];
  _lcd._prev = _lcd._last;
  _lcd._last = buf[7];
  int16 *int16buf;
  int i;
  for(i=0; i<2; i++) {
    int16buf = (int16*) &buf[CD_offset[i]];
    _new->data[i].pos[0] = int16buf[0] * INT16_FLOAT_MM * _lcd._hemi_mirror[i]; // Y and Z are interchanged and 
    _new->data[i].pos[2] = int16buf[1] * INT16_FLOAT_MM * _lcd._hemi_mirror[i]; // Z multiplied with -1 to provide
    _new->data[i].pos[1] = -int16buf[2] * INT16_FLOAT_MM * _lcd._hemi_mirror[i];// a right-hand system (GL)
    _new->data[i].quat[0] = int16buf[3] * INT16_FLOAT_M11;
    _new->data[i].quat[1] = int16buf[4] * INT16_FLOAT_M11;                      //Same change for the rotation
    _new->data[i].quat[3] = int16buf[5] * INT16_FLOAT_M11;                      //but also a 180 rotation around X
    _new->data[i].quat[2] = -int16buf[6] * INT16_FLOAT_M11;                     //after the normalization
    memcpy(&_new->data[i].buttons, &buf[CD_offset[i]+14], sizeof(ubyte));
    int16buf = (int16*) &buf[CD_offset[i]+15];
    _new->data[i].joy_x = int16buf[0] * INT16_FLOAT_M11;
    _new->data[i].joy_y = int16buf[1] * INT16_FLOAT_M11;
    _new->data[i].trigger = ((float) buf[CD_offset[i]+19]) * UINT8_FLOAT_01;
    _new->data[i].which = i;
    //Normalize quaternion
    float invlength = 1.f / sqrt(_new->data[i].quat[0] * _new->data[i].quat[0] + 
        _new->data[i].quat[1] * _new->data[i].quat[1] + 
        _new->data[i].quat[2] * _new->data[i].quat[2] +
        _new->data[i].quat[3] * _new->data[i].quat[3]);
    _new->data[i].quat[0] *= invlength;
    _new->data[i].quat[1] *= invlength;
    _new->data[i].quat[2] *= invlength;
    _new->data[i].quat[3] *= invlength;
    _quat_180Xrot(_new->data[i].quat);

    if(_lid._status & LERNA_SPHTRAC) _hemisphere_tracking(i);
    //TODO filter pos and quat
  }
}

void _startStreaming(void) {
  hid_send_feature_report(_lid._hydra_ctl, HYDRA_FEATURE_REPORT, HYDRA_FEATURE_REPORT_LEN);
  ubyte buf[91] = {0};
  hid_get_feature_report(_lid._hydra_ctl, buf, 91);

  //First reading (will take up to 4 seconds aprox for the Hydra to start sending data)
  //Set hemisphere tracking - assume controllers in dock: left controller should have negative x, right controller positive x
  _lid._status |= LERNA_SENDING;
  ubyte rbuf[512];
  int ret=-1;
  while(ret<=0) {
    ret = hid_read(_lid._hydra_dat, rbuf, sizeof(rbuf));
    if(ret>0) {
      _lcd._hemi_mirror[0] = 1.f;
      _lcd._hemi_mirror[1] = 1.f;
      _processData(rbuf);
      if(_lcd._history[_lcd._last].data[0].pos[0] > 0.f) {
        _lcd._hemi_mirror[0] = -1.f;
        _lcd._history[_lcd._last].data[0].pos[0] *= -1.f;
        _lcd._history[_lcd._last].data[0].pos[1] *= -1.f;
        _lcd._history[_lcd._last].data[0].pos[2] *= -1.f;
      }
      if(_lcd._history[_lcd._last].data[1].pos[0] < 0.f) {
        _lcd._hemi_mirror[1] = -1.f;
        _lcd._history[_lcd._last].data[1].pos[0] *= -1.f;
        _lcd._history[_lcd._last].data[1].pos[1] *= -1.f;
        _lcd._history[_lcd._last].data[1].pos[2] *= -1.f;
      }
      _lid._status |= LERNA_SPHTRAC;
    }
    else if(ret<0) fprintf(stderr, "Err\n");
  }

  time(&_lid._time);
}
void _stopStreaming(void) {
  hid_send_feature_report(_lid._hydra_ctl, HYDRA_GAMEPAD_COMMAND, HYDRA_GAMEPAD_COMMAND_LEN);
}

int _hydraProcess(void *arg){
  _startStreaming();

  ubyte rbuf[512];
  int ret;
  while(_lid._runTh) {
    ret = hid_read(_lid._hydra_dat, rbuf, sizeof(rbuf));
    if(ret>0) {
      _processData(rbuf);
    }
    else if(ret<0) fprintf(stderr, "Err\n");
  }
  return 0;
}

int _closehydra() {
  hid_close(_lid._hydra_ctl);
  hid_close(_lid._hydra_dat);
  return (!hid_exit() ? LERNA_OK : LERNA_HID_ERROR);
}

//Acquire Hydra - assume first capable device
int lernaInit(void) {
  _lid._runTh = 0;
  _lid._status = 0;
  if(!hid_init()){
    struct hid_device_info *devs = hid_enumerate(HYDRA_VENDORID, HYDRA_PRODUCTID);
    struct hid_device_info *cur_dev = devs;

    while(cur_dev) { //Expecting only two devices
      if(cur_dev->interface_number) _lid._hydra_ctl = hid_open_path(cur_dev->path);
      else _lid._hydra_dat = hid_open_path(cur_dev->path);

      cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);

    if(!_lid._hydra_ctl || !_lid._hydra_dat) {
      hid_exit();
      return LERNA_HID_ERROR;
    }
  }
  if(hid_set_nonblocking(_lid._hydra_ctl, 1) || hid_set_nonblocking(_lid._hydra_dat, 1)){
    _closehydra();
    return LERNA_HID_ERROR;
  }


  _lid._runTh = 1;
  if(thrd_success != thrd_create(&_lid._th, _hydraProcess, NULL)){
    _stopStreaming();
    _closehydra();
    return LERNA_THREADING_ERROR;
  }
  return LERNA_OK;
}

//Release Hydra
int lernaExit(void) {
  if(!_lid._runTh) return LERNA_OK;

  _lid._runTh = 0; //Stop the thread
  int badresult = thrd_success != thrd_join(_lid._th, NULL) ? LERNA_THREADING_ERROR : LERNA_OK;
  _stopStreaming();
  badresult |= _closehydra();
  
  return badresult;
}

//We require 1 second (at 250Hz) to get a full historical controller data buffer (although 1/4 would be enough for a history of 64)
//I known time() has resolution up to seconds, but if we require 1 sec, this function may be portable enough...
int lernaIsActive(void) {
  time_t t2; time(&t2);
  double elapsedTime = difftime(t2, _lid._time);
  return _lid._runTh && (_lid._status & LERNA_SENDING) && elapsedTime>1.0;
}

int lernaGetDualControllerData(lernaDualControllerData *usr) {
  if(_lid._status & LERNA_SENDING)
    memcpy(usr, &_lcd._history[_lcd._last], sizeof(lernaDualControllerData));
  else return LERNA_ERROR;
  return LERNA_OK;
}

int lernaGetControllerData(controller c, lernaControllerData *usr) {
  if(_lid._status & LERNA_SENDING)
    memcpy(usr, &_lcd._history[_lcd._last].data[c], sizeof(lernaControllerData));
  else return LERNA_ERROR;
  return LERNA_OK;
}

int lernaGetHistoryDualControllerData(unsigned char farback, lernaDualControllerData *usr) {
  if(_lid._status & LERNA_SENDING && farback<LERNA_MAX_HISTORY){
    ubyte pos = farback > _lcd._last? 255 - farback + _lcd._last : _lcd._last - farback;
    memcpy(usr, &_lcd._history[pos], sizeof(lernaDualControllerData));
    return LERNA_OK;
  }
  else return LERNA_ERROR;
}

int lernaGetHistoryControllerData(unsigned char farback, controller c, lernaControllerData *usr) {
  if(_lid._status & LERNA_SENDING && farback<LERNA_MAX_HISTORY){
    ubyte pos = farback > _lcd._last? 255 - farback + _lcd._last : _lcd._last - farback;
    memcpy(usr, &_lcd._history[pos].data[c], sizeof(lernaControllerData));
    return LERNA_OK;
  }
  else return LERNA_ERROR;
}

