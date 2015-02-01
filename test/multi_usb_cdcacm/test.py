#!/usr/bin/env python3

import serial
import os
import time

data = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
console = serial.Serial('/dev/ttyACM1', 115200, timeout=1)
test_amount = 2000
err_condat = 0
err_datcon = 0
bytes = 50

print('Testing started....')
start_time = time.time()
for i in range(0,test_amount):
  time.sleep(0.002)
  console_write = os.urandom(bytes)
  data_write = os.urandom(bytes)
  data.write(data_write)
  console.write(console_write)

  data_read = data.read(bytes)
  console_read = console.read(bytes)
  if data_read != console_write:
    err_condat += 1
    print('\tConsole -> Data error')
    print('\t\tWritten Console:', console_write)
    print('\t\tRead Data:', data_read)
    print('\t\tWritten Data:', data_write)
    print('\t\tRead Console:', console_read)
    break
  if console_read != data_write:
    err_datcon += 1
    print('\tData -> Console error')
    print('\t\tWritten Data:', data_write)
    print('\t\tRead Console:', console_read)
    print('\t\tWritten Console:', console_write)
    print('\t\tRead Data:', data_read)
    break


i += 1
end_time = time.time()
print('Testing complete of',i,'tests of',bytes,'bytes in', end_time-start_time, 'seconds!')
print(i/(end_time-start_time), 'writes/reads per second,', (end_time-start_time)/i*1000, 'ms per write/read')
print(bytes*i/(end_time-start_time)/1024, 'Kb/s (from Console<->Data, so *2 for all)')
print('Errors:')
print('\tConsole -> Data:', err_condat)
print('\tData -> Console:', err_datcon)
data.close();
console.close();
