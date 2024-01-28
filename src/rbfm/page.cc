#include "src/include/page.h"
#include "src/include/util.h"
#include <cstring>

namespace PeterDB {
    Page::Page() {
        initPageMetadata();
    }

    Page::~Page() {
        delete[] m_data;
        m_data = nullptr;
    }

    void Page::initPageMetadata() {
        setFreeByteCount(PAGE_SIZE - PAGE_METADATA_SIZE);
        setSlotCount(0);
    }

    bool Page::canInsertRecord(unsigned short recordLengthBytes) {
        unsigned short availableBytes = getFreeByteCount();
        // account for the new slot metadata that we need to write after inserting a new record
        return availableBytes >= (recordLengthBytes + Slot::SLOT_LENGTH_BYTES);
    }

    unsigned short Page::insertRecord(void *recordData, unsigned short recordLengthBytes) {
        if (!canInsertRecord(recordLengthBytes)) {
            ERROR("Cannot insert record of size=%hu into page having %hu bytes free\n", recordLengthBytes,
                  getFreeByteCount());
            return -1;
        }

        // insert the record data into the page
        unsigned short slotNumber = getSlotForInsertion(recordLengthBytes);
        unsigned short recordOffset = computeRecordOffset(slotNumber);
        byte *recordStart = m_data + recordOffset;
        memcpy(recordStart, recordData, recordLengthBytes);

        // set the newly inserted record's slot metadata
        getSlot(slotNumber).setRecordOffsetBytes(recordOffset);
        getSlot(slotNumber).setRecordLengthBytes(recordLengthBytes);

        // update the page's metadata
        if (slotNumber == getSlotCount()) {
            // this was a newly created slot.
            setSlotCount(getSlotCount() + 1);
        }
        setFreeByteCount(getFreeByteCount() - recordLengthBytes - Slot::SLOT_LENGTH_BYTES);

        INFO("Inserted record of length=%hu into slot=%hu. Free bytes avlbl=%hu\n", recordLengthBytes, slotNumber, getFreeByteCount());
        return slotNumber;
    }

    void Page::readRecord(unsigned short slotNumber, void *data) {
        Slot recordSlot = getSlot(slotNumber);
        if (recordSlot.getRecordLengthBytes() == 0) {
            return; // this was a deleted record
        }
        void *recordDataStart = (void*) (m_data + recordSlot.getRecordOffsetBytes());
        memcpy(data, recordDataStart, recordSlot.getRecordLengthBytes());
    }

    void Page::deleteRecord(unsigned short slotNumber) {
        assert(slotNumber >= 0 && slotNumber < getSlotCount());
        Slot recordSlot = getSlot(slotNumber);

//        shift records from subsequent slots (if any) left by the length of the deleted record
        shiftRecordsLeft(slotNumber + 1, recordSlot.getRecordLengthBytes());

//          update the page's freeByteCount
//          Note: the slot of the deleted record, and the slot's metadata size,
//          is 'lost' (can never be used) forever.
        setFreeByteCount(getFreeByteCount() + recordSlot.getRecordLengthBytes());

//      Set the record to be deleted'd length = 0 in the slot directory.
        recordSlot.setRecordLengthBytes(0);
    }

    void Page::eraseAndReset() {
        if (nullptr != m_data)
            memset((void *) m_data, 0, PAGE_SIZE);
        initPageMetadata();
    }

    unsigned short Page::getFreeByteCount() {
        return *freeByteCount;
    }

    void Page::setFreeByteCount(unsigned short numBytesFree) {
        *freeByteCount = numBytesFree;
    }

    unsigned short Page::getSlotCount() {
        return *slotCount;
    }

    void Page::setSlotCount(unsigned short numSlotsInPage) {
        *slotCount = numSlotsInPage;
    }

    void *Page::getDataPtr() {
        return (void *) m_data;
    }

    unsigned short Page::computeRecordOffset(unsigned short slotNumber) {
        if (slotNumber == 0) {
            return FIRST_RECORD_OFFSET;
        }
        Slot previousSlot = getSlot(slotNumber - 1);
        return previousSlot.getRecordOffsetBytes() + previousSlot.getRecordLengthBytes();
    }

    Slot Page::getSlot(unsigned short slotNum){
        Slot slot((void *) (slotMetadataEnd - (Slot::SLOT_LENGTH_BYTES * (slotNum + 1))));
        return slot;
    }

    void Page::shiftRecordsLeft(int slotNumStart, unsigned short shiftOffsetBytes) {
        if (slotNumStart >= getSlotCount()) {
            return;
        }

        for (unsigned short slotNum = slotNumStart; slotNum < getSlotCount(); ++slotNum) {
            Slot slot = getSlot(slotNum);
            unsigned short recordOffsetOld = slot.getRecordOffsetBytes();
            unsigned short recordOffsetNew = recordOffsetOld - shiftOffsetBytes;
            memmove(m_data + recordOffsetNew, m_data + recordOffsetOld, slot.getRecordLengthBytes());

            slot.setRecordOffsetBytes(recordOffsetNew);
        }
    }

    void Page::shiftRecordsRight(int slotNumStart, unsigned short shiftOffsetBytes) {
        if (slotNumStart >= getSlotCount()) {
            return;
        }

        for (unsigned short slotNum = getSlotCount() - 1; slotNum >= slotNumStart; --slotNum) {
            Slot slot = getSlot(slotNum);
            unsigned short recordOffsetOld = slot.getRecordOffsetBytes();
            unsigned short recordOffsetNew = recordOffsetOld + shiftOffsetBytes;
            memmove(m_data + recordOffsetNew, m_data + recordOffsetOld, slot.getRecordLengthBytes());

            slot.setRecordOffsetBytes(recordOffsetNew);
        }
    }

    unsigned short Page::getRecordLengthBytes(unsigned short slotNumber) {
        return getSlot(slotNumber).getRecordLengthBytes();
    }

    unsigned short Page::getSlotForInsertion(unsigned short recordLengthBytes) {
        if (getSlotCount() == 0) {
            return 0;
        }
        for (unsigned short slotNum = 0; slotNum < getSlotCount(); ++slotNum) {
            Slot slot = getSlot(slotNum);
            if (slot.getRecordLengthBytes() == 0) {
                shiftRecordsRight(slotNum + 1, recordLengthBytes);
                return slotNum;
            }
        }
        return getSlotCount();
    }
}