#!/usr/bin/python
# -*- coding: utf-8 -*-
import time
import datetime
import serial

START_CODE = 0x30
END_CODE = 0x20
ESC_CODE = 0xF0
START_REP_CODE = 0xF1
END_REP_CODE = 0xF2
ESC_REP_CODE = 0xF3

COMMAND_ID_SET_TIME = '\x00'


def pack_data(data):
    _data = chr(START_CODE)
    for c in data:
        if ord(c) == START_CODE:
            c = chr(ESC_CODE) + chr(START_REP_CODE)
        elif ord(c) == END_CODE:
            c = chr(ESC_CODE) + chr(END_REP_CODE)
        elif ord(c) == ESC_CODE:
            c = chr(ESC_CODE) + chr(ESC_REP_CODE)
        _data += c
    _data += chr(END_CODE)
    return _data


def serial_send(data):
    ser = serial.Serial(port='/dev/ttyUSB0', baudrate=57600)
    data = pack_data(data)
    ser.write(data)
    ser.close()


def set_time(year, month, day, hour, minute, second):
    if year < 100:
        if year < 50:
            year_full = year + 2000
        else:
            year_full = year + 1900
    else:
        year_full = year
        year = year % 100
    week_day = datetime.datetime(year_full, month, day, hour, minute, second).weekday()
    # correct week day
    week_day += 2
    if week_day == 8:
        week_day = 1
    cur_time = '{:02d}{:02d}{:02d}{:02d}{:02d}{:02d}{:02d}'.format(second, minute, hour, week_day, day, month, year)
    data = COMMAND_ID_SET_TIME + cur_time
    serial_send(data)


def set_cur_time():
    cur_time = time.strftime('%S%M%H0%w%d%m%y')
    cur_time = list(cur_time)
    cur_time[7] = chr(ord(cur_time[7]) + 1)  # correct week day
    cur_time = ''.join(cur_time)
    data = COMMAND_ID_SET_TIME + cur_time
    serial_send(data)


if __name__ == "__main__":
    set_cur_time()
    # set_time(2016, 10, 30, 3, 59, 45)
    # set_time(2016, 3, 27, 2, 59, 45)
