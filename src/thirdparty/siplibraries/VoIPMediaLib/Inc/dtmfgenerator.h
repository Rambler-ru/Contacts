/*
* $Id: dtmfgenerator.h,v 1.2 2005/05/18 07:06:14 pera Exp $
*
* (c) 2003 iptel.org
*
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU Library General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
* 
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
* License for more details.
* 
* You should have received a copy of the GNU Library General Public License
* along with this library; see the file COPYING.LIB.  If not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
* MA 02111-1307, USA.
*
*/

#ifndef DTMFGENERATOR_H
#define DTMFGENERATOR_H

#include "voipmedia_global.h"

#include <exception>
#include <string.h>

#define NUM_TONES 16  // was 30

/*
* DMTF Generator Exception
*/
class VOIPMEDIA_EXPORT DTMFException : public std::exception
{
private:
  const char* reason;
public:
  DTMFException(const char* _reason) throw();
  virtual ~DTMFException() throw();
  virtual const char* what() const throw();
};


/*
* DTMF Tone Generator
*/
class VOIPMEDIA_EXPORT DTMFGenerator
{
private:
  unsigned int samplingRate; // Sampling rate used, default is 8000 Hz
  short amplitude;           // Amplitude of the resulting signal

  struct DTMFTone
  {
    unsigned char code; // Code of the tone
    int lower;          // Lower frequency
    int higher;         // Higher frequency
  };

  /*
  * State of the DTMF generator
  */
  struct DTMFState
  {
    unsigned int offset;   // Offset in the sample currently being played
    short* sample;         // Currently generated code
  };

  DTMFState state;
  static const DTMFTone tones[NUM_TONES];

  short* samples[NUM_TONES];        // Generated samples

public:
  DTMFGenerator();
  ~DTMFGenerator();

  /*
  * Get n samples of the signal of code code
  */
  void getSamples(short* buffer, size_t n, unsigned char code) throw (DTMFException);

  /*
  * Get next n samples (continues where previous call to
  * genSample or genNextSamples stopped
  */
  void getNextSamples(short* buffer, size_t n) throw (DTMFException);

private:
  short* generateSample(unsigned char code) throw (DTMFException);

};


#endif // DTMFGENERATOR_H
