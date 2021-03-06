/**
 ******************************************************************************
  Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
*/

#ifndef SYSTEM_TEST_H_
#define SYSTEM_TEST_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void testDigitalWrite(uint16_t pin, uint16_t value, void* cookie);
void testAnalogRead(uint16_t pin, void* cookie);
void testSelfTest(void* cookie);
void testRfCheck(void* cookie);
void testSensorData(void* cookie);

#endif	/* __SYSTEM_TEST_H */

