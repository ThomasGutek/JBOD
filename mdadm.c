#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "jbod.h"
#include "mdadm.h"

int isMounted = 0; // 0 is false

uint32_t encode_op(int cmd, int disk_num, int block_num) {
    uint32_t op = 0;
    op |= (cmd << 26);
    op |= (disk_num << 22);
    op |= (block_num << 0);
    return op;
}

int mdadm_mount(void) {
    if (isMounted == 1) return -1;
    uint32_t op = encode_op(JBOD_MOUNT, 0, 0);
    jbod_client_operation(op, NULL);
    isMounted = 1;
    return 1;
}

int mdadm_unmount(void) {
    if (isMounted == 0) return -1;
    uint32_t op = encode_op(JBOD_UNMOUNT, 0, 0);
    jbod_client_operation(op, NULL);
    isMounted = 0;
    return 1;
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {
    // check parameters
    if (addr == 0 && len == 0 && buf == NULL) return 0;
    if (isMounted != 1 || addr >= 1048575 || (len + addr) > 1048575 ||
        len > 1024 || buf == NULL)
        return -1;

    int num_read = 0;
    int templen = len;
    while (len > 0) {
        // calculate disk, block number and offset
        uint32_t disknum = addr / 65536;
        uint32_t blocknum = (addr % 65536) / 256;
        int offset = addr % 256;

        // seek operations
        jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disknum, blocknum),
                              NULL);
        jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, disknum, blocknum),
                              NULL);

        // read block in to mybuf
        uint8_t mybuf[256];
        jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, 0), mybuf);

        // figure out how many bytes to read to copy from this block to user
        // buffer
        // int num_bytes_to_read_from_block = min(len, min(256, 256 - offset));

        // copy bytes to user buffer
        // if first iteration
        if (num_read == 0) {
            // within block
            if ((offset + len) <= 256) {
                memcpy(buf, mybuf + offset, len);
                // increment the number of bytes read, decrement the number of
                // bytes remaining to be read
                num_read += (len);
                len -= (len);
                // advance the addr by that number of bytes to go to the next
                // address to read from
                addr += (len);
            }
            // if length spans past current block, copy the whole block
            else {
                memcpy(buf, mybuf + offset, 256 - offset);
                // increment the number of bytes read, decrement the number of
                // bytes remaining to be read
                num_read += (256 - offset);
                len -= (256 - offset);
                // advance the addr by that number of bytes to go to the next
                // address to read from
                addr += (256 - offset);
            }
        }
        // if not first iteration
        else
            // last block
            if ((len) <= 256) {
                memcpy(buf + num_read, mybuf, len);
                // increment the number of bytes read, decrement the number of
                // bytes remaining to be read
                num_read += (len);
                len -= (len);
                // advance the addr by that number of bytes to go to the next
                // address to read from
                addr += (len);
            }
            // if length spans past current block, copy the whole block
            else {
                memcpy(buf + num_read, mybuf, 256);
                // increment the number of bytes read, decrement the number of
                // bytes
                // remaining to be read
                num_read += (256);
                len -= (256);
                // advance the addr by that number of bytes to go to the next
                // address to read from
                addr += (256);
            }
    }
    return templen;
}

int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {

    // check parameters
    if (addr == 0 && len == 0 && buf == NULL) return 0;
    if (isMounted != 1 || addr >= 1048576 || (len + addr) > 1048576 ||
        len > 1024 || buf == NULL)
        return -1;
    int num_write = 0;
    uint32_t templen = len;
    uint8_t mybuf[256];
    while (len > 0) {
        // where is address?
        uint32_t disknum = addr / 65536;
        uint32_t blocknum = (addr % 65536) / 256;
        int offset = addr % 256;

        // seek operations
        jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disknum, blocknum),
                              NULL);
        jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, disknum, blocknum),
                              NULL);

        // read
        jbod_client_operation(encode_op(JBOD_READ_BLOCK, disknum, blocknum),
                              mybuf);

        if (num_write == 0) {
            // within block
            if ((offset + len) <= 256) {
                memcpy(mybuf + offset, buf, len);
                // increment the number of bytes read, decrement the number of
                // bytes remaining to be read
                num_write += (len);
                len -= (len);
                // advance the addr by that number of bytes to go to the next
                // address to read from
                addr += (len);
            }
            // if length spans past current block, copy the whole block
            else {
                memcpy(mybuf + offset, buf, 256 - offset);
                // increment the number of bytes read, decrement the number of
                // bytes remaining to be read
                num_write += (256 - offset);
                len -= (256 - offset);
                // advance the addr by that number of bytes to go to the next
                // address to read from
                addr += (256 - offset);
            }
        }
        // if not first iteration
        else
            // last block
            if ((len) <= 256) {
                memcpy(mybuf, buf + num_write, len);
                // increment the number of bytes read, decrement the number of
                // bytes remaining to be read
                num_write += (len);
                len -= (len);
                // advance the addr by that number of bytes to go to the next
                // address to read from
                addr += (len);
            }
            // if length spans past current block, copy the whole block
            else {
                memcpy(mybuf, buf + num_write, 256);
                // increment the number of bytes read, decrement the number of
                // bytes
                // remaining to be read
                num_write += (256);
                len -= (256);
                // advance the addr by that number of bytes to go to the next
                // address to read from
                addr += (256);
            }

        // seek operations
        jbod_client_operation(encode_op(JBOD_SEEK_TO_DISK, disknum, blocknum),
                              NULL);
        jbod_client_operation(encode_op(JBOD_SEEK_TO_BLOCK, disknum, blocknum),
                              NULL);

        // write block
        jbod_client_operation(encode_op(JBOD_WRITE_BLOCK, disknum, blocknum),
                              mybuf);
    }

    return templen;
}