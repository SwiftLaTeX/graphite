#pragma once

#include "graphiteng/Types.h"
#include "graphiteng/SegmentHandle.h"



class NoLimit		//relies on the processor.processChar() failing, such as because of a terminating nul character
{
public:
    static bool inBuffer(const void* pCharLastSurrogatePart) { return true; }
    static bool needMoreChars(const void* pCharStart, size_t nProcessed) { return true; }
};


class CharacterCountLimit
{
public:
    CharacterCountLimit(size_t numchars) : m_numchars(numchars) {}
  
    static bool inBuffer(const void* pCharLastSurrogatePart) { return true; }
    bool needMoreChars(const void* pCharStart, size_t nProcessed) const { return nProcessed<m_numchars; }
    
private:
    size_t m_numchars;
};


class BufferLimit
{
public:
    BufferLimit(const void* pEnd/*as in stl i.e. don't use end*/) : m_pEnd(pEnd) {}
  
    bool inBuffer(const uint8* pCharLastSurrogatePart) const { return pCharLastSurrogatePart<m_pEnd; }	//also called on charstart by needMoreChars()
    bool inBuffer(const uint16* pCharLastSurrogatePart) const { return pCharLastSurrogatePart<static_cast<const void*>(static_cast<const char*>(m_pEnd)-1/*to allow for the second byte of pCharLastSurrogatePart*/); }	//also called on charstart by needMoreChars()
    bool inBuffer(const uint32* pCharStart) const { return pCharStart<static_cast<const void*>(static_cast<const char*>(m_pEnd)-3/*to allow for the fourth byte of pCharStart*/); }	//fwiw utf does not have surrogates. also called on charstart by needMoreChars()

    template <class UINT>
    bool needMoreChars(const UINT* pCharStart, size_t nProcessed) const { return inBuffer(pCharStart); }
     
private:
    const void* m_pEnd;
};


class BufferAndCharacterCountLimit : public BufferLimit
{
public:
    BufferAndCharacterCountLimit(const void* pEnd/*as in stl i.e. don't use end*/, size_t numchars) : BufferLimit(pEnd), m_numchars(numchars) {}
  
    //inBuffer is inherited for conveniently 
    template <class UINT>
    bool needMoreChars(const UINT* pCharStart, size_t nProcessed) const { return nProcessed<m_numchars && inBuffer(pCharStart); }
     
private:
    size_t m_numchars;
};


const int utf8_sz_lut[16] = {1,1,1,1,1,1,1,        // 1 byte
                                          0,0,0,0,  // trailing byte
                                          2,2,            // 2 bytes
                                          3,                 // 3 bytes
                                          4};                // 4 bytes

const byte utf8_mask_lut[5] = {0x80,0x00,0xC0,0xE0,0xF0};

class Utf8Consumer
{
public:
      Utf8Consumer(const uint8* pCharStart2) : m_pCharStart(pCharStart2) {}
      
      const uint8* pCharStart() const { return m_pCharStart; }
  
      template <class LIMIT>
      inline bool consumeChar(const LIMIT& limit, uint32* pRes)			//At start, limit.inBuffer(m_pCharStart) is true. return value is iff character contents does not go past limit
      {
	const size_t    seq_sz = utf8_sz_lut[*m_pCharStart >> 4];
	if (seq_sz==0) {
	    *pRes = 0xFFFD;
	    return true;			//this is an error. But carry on anyway?
	}
	
	if (!limit.inBuffer(m_pCharStart+(seq_sz-1))) {
	    return false;
	}
	
	*pRes = *m_pCharStart ^ utf8_mask_lut[seq_sz];
	
	switch(seq_sz) {      
	    case 4:     *pRes <<= 6; *pRes |= *++m_pCharStart & 0x7F;
	    case 3:     *pRes <<= 6; *pRes |= *++m_pCharStart & 0x7F;
	    case 2:     *pRes <<= 6; *pRes |= *++m_pCharStart & 0x7F; break;
	    case 1: default:    break;
	}
	++m_pCharStart; 
	return true;
      }	
  
private:
      const uint8 *m_pCharStart;
};



class Utf16Consumer
{
private:
    static const int SURROGATE_OFFSET = 0x10000 - (0xD800 << 10) - 0xDC00;

public:
      Utf16Consumer(const uint16* pCharStart2) : m_pCharStart(pCharStart2) {}
      
      const uint16* pCharStart() const { return m_pCharStart; }
  
      template <class LIMIT>
      inline bool consumeChar(const LIMIT& limit, uint32* pRes)			//At start, limit.inBuffer(m_pCharStart) is true. return value is iff character contents does not go past limit
      {
	  *pRes = *(m_pCharStart)++;
	  if (*pRes > 0xDBFF || 0xD800 > *pRes)
	      return true;

	  if (!limit.inBuffer(m_pCharStart+1)) {
	      return false;
	  }

	  uint32 ul = *(m_pCharStart++);
	  if (0xDC00 > ul || ul > 0xDFFF) {
	      *pRes = 0xFFFD;
	      return true; 			//this is an error. But carry on anyway?
	  }
	  *pRes =  (*pRes<<10) + ul - SURROGATE_OFFSET;
	  return true;
      }

private:
      const uint16 *m_pCharStart;
};


class Utf32Consumer
{
public:
      Utf32Consumer(const uint32* pCharStart2) : m_pCharStart(pCharStart2) {}
      
      const uint32* pCharStart() const { return m_pCharStart; }
  
      template <class LIMIT>
      inline bool consumeChar(const LIMIT& limit, uint32* pRes)			//At start, limit.inBuffer(m_pCharStart) is true. return value is iff character contents does not go past limit
      {
	  *pRes = *(m_pCharStart++);
	  return true;
      }

private:
      const uint32 *m_pCharStart;
};




/* The following template function assumes that LIMIT and CHARPROCESSOR have the following methods and semantics:

class LIMIT
{
public:
    bool inBuffer(const uint8* pCharLastSurrogatePart) const;	//whether or not the input is considered to be in the range of the buffer.
    bool inBuffer(const uint16* pCharLastSurrogatePart) const;	//whether or not the input is considered to be in the range of the buffer.

    bool needMoreChars(const uint8* pCharStart, size_t nProcessed) const; //whether or not the input is considered to be in the range of the buffer, and sufficient characters have been processed.
    bool needMoreChars(const uint16* pCharStart, size_t nProcessed) const; //whether or not the input is considered to be in the range of the buffer, and sufficient characters have been processed.
    bool needMoreChars(const uint32* pCharStart, size_t nProcessed) const; //whether or not the input is considered to be in the range of the buffer, and sufficient characters have been processed.
};

class CHARPROCESSOR
{
public:
    bool processChar(uint32 cid);		//return value indicates if should stop processing
    size_t charsProcessed() const;	//number of characters processed. Usually starts from 0 and incremented by processChar(). Passed in to LIMIT::needMoreChars
};

Useful reusable examples of LIMIT are:
NoLimit		//relies on the CHARPROCESSOR.processChar() failing, such as because of a terminating nul character
CharacterCountLimit //doesn't care about where the input buffer may end, but limits the number of unicode characters processed.
BufferLimit	//processes how ever many characters there are until the buffer end. characters straggling the end are not processed.
BufferAndCharacterCountLimit //processes a maximum number of characters there are until the buffer end. characters straggling the end are not processed.
*/

template <class LIMIT, class CHARPROCESSOR>
void processUTF(SegmentHandle::encform enc, const void* pStart, const LIMIT& limit/*when to stop processing*/, CHARPROCESSOR* pProcessor)
{
     uint32             cid;
     switch (enc) {
       case SegmentHandle::kutf8 : {
	    Utf8Consumer consumer(static_cast<const uint8 *>(pStart));
            for (;limit.needMoreChars(consumer.pCharStart(), pProcessor->charsProcessed());) {
		if (!consumer.consumeChar(limit, &cid))
		    break;
		if (!pProcessor->processChar(cid))
		    break;
            }
            break;
        }
       case SegmentHandle::kutf16: {
            Utf16Consumer consumer(static_cast<const uint16 *>(pStart));
            for (;limit.needMoreChars(consumer.pCharStart(), pProcessor->charsProcessed());) {
		if (!consumer.consumeChar(limit, &cid))
		    break;
		if (!pProcessor->processChar(cid))
		    break;
            }
	    break;
        }
       case SegmentHandle::kutf32 : default: {
	    Utf32Consumer consumer(static_cast<const uint32 *>(pStart));
            for (;limit.needMoreChars(consumer.pCharStart(), pProcessor->charsProcessed());) {
		if (!consumer.consumeChar(limit, &cid))
		    break;
		if (!pProcessor->processChar(cid))
		    break;
            }
            break;
        }
    }
}



