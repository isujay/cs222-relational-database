#ifndef _page_h_
#define _page_h_

#define PAGE_SIZE 4096 //todo: consolidate with pfm's definition
#define PAGE_METADATA_SIZE sizeof(unsigned short) * 2
#define FIRST_RECORD_OFFSET 0

#include <cstdlib>
#include "src/include/slot.h"

typedef unsigned char byte;

namespace PeterDB {
    class Page {

    public:
        Page();
        ~Page();

        void initPageMetadata(); // used to create an (initially) zero records page.

        void* getDataPtr();

        bool canInsertRecord(unsigned short recordLengthBytes);

        unsigned short insertRecord(void *recordData, unsigned short recordLengthBytes);

        void readRecord(unsigned short slotNumber, void* data);

        void deleteRecord(unsigned short slotNumber);

        unsigned short getRecordLengthBytes(unsigned short slotNumber);

        void eraseAndReset();

    private:
        byte *m_data = new byte[PAGE_SIZE];
        unsigned short* freeByteCount = (unsigned short *) (m_data + PAGE_SIZE - PAGE_METADATA_SIZE) + 1;
        unsigned short* slotCount = (unsigned short *) (m_data + PAGE_SIZE - PAGE_METADATA_SIZE);
        byte *slotMetadataEnd =  (m_data + PAGE_SIZE - PAGE_METADATA_SIZE);

        unsigned short getFreeByteCount();

        unsigned short getSlotCount();

        unsigned short computeRecordOffset(unsigned short slotNumber);

        Slot getSlot(unsigned short slotNum);

        void setFreeByteCount(unsigned short numBytesFree);

        void setSlotCount(unsigned short numSlotsInPage);

        void shiftRecordsLeft(int slotNumStart, unsigned short shiftOffsetBytes);

        void shiftRecordsRight(int slotNumStart, unsigned short shiftOffsetBytes);

        unsigned short getSlotForInsertion(unsigned short recordLengthBytes);
    };
}

#endif