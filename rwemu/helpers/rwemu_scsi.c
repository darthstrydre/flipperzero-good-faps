#include "rwemu_scsi.h"

#include <core/log.h>

#define TAG "RWEmuSCSI"

#define SCSI_TEST_UNIT_READY        (0x00)
#define SCSI_REQUEST_SENSE          (0x03)
#define SCSI_INQUIRY                (0x12)
#define SCSI_READ_FORMAT_CAPACITIES (0x23)
/* #define SCSI_READ_CAPACITY_10    (0x25) */ /* Not valid for CD-ROM; stubbed to error */
#define SCSI_MODE_SENSE_6           (0x1A)
#define SCSI_READ_10                (0x28)
#define SCSI_PREVENT_MEDIUM_REMOVAL (0x1E)
#define SCSI_START_STOP_UNIT        (0x1B)
/* #define SCSI_WRITE_10            (0x2A) */ /* Not used by CD-ROM devices */

/* CD-ROM specific commands */
#define SCSI_FORMAT_UNIT   (0x04)  /* Stub: returns ILLEGAL REQUEST */
#define SCSI_READ_TOC      (0x43)  /* Table of Contents / PMA / ATIP */
#define SCSI_GET_CONFIG    (0x46)  /* Get Configuration */
#define SCSI_READ_DISC_INFO (0x51) /* Read Disc Information */
#define SCSI_READ_TRACK_INFO (0x52) /* Read Track Information */
#define SCSI_READ_DVD_STRUCT (0xAD) /* Read DVD Structure */
#define SCSI_READ_12       (0xA8)  /* READ(12) command */
#define SCSI_BLANK         (0xA1)  /* Blank CD stub: returns ILLEGAL REQUEST */
#define SCSI_READ_CD       (0xBE)  /* Variable sector size read with sub-channel */

bool scsi_cmd_start(SCSISession* scsi, uint8_t* cmd, uint8_t len) {
    if(!len) {
        scsi->sk = SCSI_SK_ILLEGAL_REQUEST;
        scsi->asc = SCSI_ASC_INVALID_COMMAND_OPERATION_CODE;
        return false;
    }
    FURI_LOG_T(TAG, "START %02X", cmd[0]);
    scsi->cmd = cmd;
    scsi->cmd_len = len;
    scsi->rx_done = false;
    scsi->tx_done = false;
    switch(cmd[0]) {
    case SCSI_READ_10: {
        if(len < 10) return false;
        scsi->read_10.lba = cmd[2] << 24 | cmd[3] << 16 | cmd[4] << 8 | cmd[5];
        scsi->read_10.count = cmd[7] << 8 | cmd[8];
        FURI_LOG_D(TAG, "SCSI_READ_10 %08lX %04X", scsi->read_10.lba, scsi->read_10.count);
        return true;
    }; break;
    case SCSI_READ_12: {
        if(len < 12) return false;
        scsi->read_10.lba = cmd[2] << 24 | cmd[3] << 16 | cmd[4] << 8 | cmd[5];
        scsi->read_10.count = (cmd[6] << 16) | (cmd[7] << 8) | cmd[8];
        FURI_LOG_D(TAG, "SCSI_READ_12 %08lX %04X", scsi->read_10.lba, scsi->read_10.count);
        return true;
    }; break;
    case SCSI_READ_TOC: {
        if(len < 8) return false;
        scsi->tx_done = false;
        FURI_LOG_D(TAG, "SCSI_READ_TOC format=%02X", cmd[1]);
        return true;
    }; break;
    case SCSI_GET_CONFIG: {
        if(len < 6) return false;
        scsi->tx_done = false;
        FURI_LOG_D(TAG, "SCSI_GET_CONFIG");
        return true;
    }; break;
    case SCSI_READ_DISC_INFO: {
        if(len < 4) return false;
        scsi->tx_done = false;
        FURI_LOG_D(TAG, "SCSI_READ_DISC_INFO");
        return true;
    }; break;
    case SCSI_READ_TRACK_INFO: {
        if(len < 6) return false;
        scsi->tx_done = false;
        FURI_LOG_D(TAG, "SCSI_READ_TRACK_INFO track=%02X", cmd[1]);
        return true;
    }; break;
    case SCSI_READ_DVD_STRUCT: {
        if(len < 6) return false;
        scsi->tx_done = false;
        FURI_LOG_D(TAG, "SCSI_READ_DVD_STRUCT type=%02X", cmd[2]);
        return true;
    }; break;
    case SCSI_READ_CD: {
        if(len < 10) return false;
        scsi->read_10.lba = cmd[2] << 24 | cmd[3] << 16 | cmd[4] << 8 | cmd[5];
        scsi->read_10.count = cmd[7] << 8 | cmd[8];
        FURI_LOG_D(TAG, "SCSI_READ_CD %08lX %04X", scsi->read_10.lba, scsi->read_10.count);
        return true;
    }; break;
    }
    return true;
}

bool scsi_cmd_rx_data(SCSISession* scsi, uint8_t* data, uint32_t len) {
    FURI_LOG_T(TAG, "RX %02X len %lu", scsi->cmd[0], len);
    if(scsi->rx_done) return false;
    switch(scsi->cmd[0]) {
    default: {
        FURI_LOG_W(TAG, "unexpected scsi rx data cmd=%02X", scsi->cmd[0]);
        scsi->sk = SCSI_SK_ILLEGAL_REQUEST;
        scsi->asc = SCSI_ASC_INVALID_COMMAND_OPERATION_CODE;
        return false;
    }; break;
    }
}

bool scsi_cmd_tx_data(SCSISession* scsi, uint8_t* data, uint32_t* len, uint32_t cap) {
    FURI_LOG_T(TAG, "TX %02X cap %lu", scsi->cmd[0], cap);
    if(scsi->tx_done) return false;
    switch(scsi->cmd[0]) {
    case SCSI_REQUEST_SENSE: {
        FURI_LOG_D(TAG, "SCSI_REQUEST_SENSE");
        if(cap < 18) return false;
        memset(data, 0, cap);
        data[0] = 0x70; // fixed format sense data
        data[1] = 0; // obsolete
        data[2] = scsi->sk; // sense key
        data[3] = 0; // information
        data[4] = 0; // information
        data[5] = 0; // information
        data[6] = 0; // information
        data[7] = 10; // additional sense length (len-8)
        data[8] = 0; // command specific information
        data[9] = 0; // command specific information
        data[10] = 0; // command specific information
        data[11] = 0; // command specific information
        data[12] = scsi->asc; // additional sense code
        data[13] = 0; // additional sense code qualifier
        data[14] = 0; // field replaceable unit code
        data[15] = 0; // sense key specific information
        data[16] = 0; // sense key specific information
        data[17] = 0; // sense key specific information
        *len = 18;
        scsi->sk = 0;
        scsi->asc = 0;
        scsi->tx_done = true;
        return true;
    }; break;
    case SCSI_INQUIRY: {
        FURI_LOG_D(TAG, "SCSI_INQUIRY");
        if(scsi->cmd_len < 5) return false;

        if(cap < 36) return false;

        bool evpd = scsi->cmd[1] & 1;
        uint8_t page_code = scsi->cmd[2];
        if(evpd == 0) {
            if(page_code != 0) return false;

            data[0] = 0x05; // device type: CD/DVD
            data[1] = 0x80; // removable: true
            data[2] = 0x04; // version
            data[3] = 0x02; // response data format
            data[4] = 31; // additional length (len - 5)
            data[5] = 0; // flags
            data[6] = 0; // flags
            data[7] = 0; // flags
            memcpy(data + 8, "Flipper ", 8); // vendor id
            memcpy(data + 16, "RW Emu          ", 16); // product id
            memcpy(data + 32, "0001", 4); // product revision level
            *len = 36;
            scsi->tx_done = true;
            return true;
        } else {
            if(page_code != 0x80) {
                FURI_LOG_W(TAG, "Unsupported VPD code %02X", page_code);
                return false;
            }
            data[0] = 0x00;
            data[1] = 0x80;
            data[2] = 0x00;
            data[3] = 0x01; // Serial len
            data[4] = '0';
            *len = 5;
            scsi->tx_done = true;
            return true;
        }
    }; break;
    case SCSI_READ_FORMAT_CAPACITIES: {
        FURI_LOG_D(TAG, "SCSI_READ_FORMAT_CAPACITIES");
        if(cap < 12) {
            return false;
        }
        uint32_t n_blocks = scsi->fn.num_blocks(scsi->fn.ctx);
        uint32_t block_size = SCSI_BLOCK_SIZE;
        // Capacity List Header
        data[0] = 0;
        data[1] = 0;
        data[2] = 0;
        data[3] = 8;

        // Capacity Descriptor
        data[4] = (n_blocks - 1) >> 24;
        data[5] = (n_blocks - 1) >> 16;
        data[6] = (n_blocks - 1) >> 8;
        data[7] = (n_blocks - 1) & 0xFF;
        data[8] = 0x02; // Formatted media
        data[9] = block_size >> 16;
        data[10] = block_size >> 8;
        data[11] = block_size & 0xFF;
        *len = 12;
        scsi->tx_done = true;
        return true;
    }; break;
    /*
    case SCSI_READ_CAPACITY_10: {
        FURI_LOG_D(TAG, "SCSI_READ_CAPACITY_10");
        if(cap < 8) return false;
        uint32_t n_blocks = scsi->fn.num_blocks(scsi->fn.ctx);
        uint32_t block_size = SCSI_BLOCK_SIZE;
        data[0] = (n_blocks - 1) >> 24;
        data[1] = (n_blocks - 1) >> 16;
        data[2] = (n_blocks - 1) >> 8;
        data[3] = (n_blocks - 1) & 0xFF;
        data[4] = block_size >> 24;
        data[5] = block_size >> 16;
        data[6] = block_size >> 8;
        data[7] = block_size & 0xFF;
        *len = 8;
        scsi->tx_done = true;
        return true;
    }; break
    */
    case SCSI_MODE_SENSE_6: {
        FURI_LOG_D(TAG, "SCSI_MODE_SENSE_6 %lu", cap);
        if(cap < 4) return false;
        data[0] = 3; // mode data length (len - 1)
        data[1] = 0; // medium type
        data[2] = 0; // device-specific parameter
        data[3] = 0; // block descriptor length
        *len = 4;
        scsi->tx_done = true;
        return true;
    }; break;
    case SCSI_READ_10: {
        uint32_t block_size = SCSI_BLOCK_SIZE;
        bool result =
            scsi->fn.read(scsi->fn.ctx, scsi->read_10.lba, scsi->read_10.count, data, len, cap);
        *len -= *len % block_size;
        uint16_t blocks = *len / block_size;
        scsi->read_10.lba += blocks;
        scsi->read_10.count -= blocks;
        if(!scsi->read_10.count) {
            scsi->tx_done = true;
        }
        return result;
    }; break;
    case SCSI_READ_12: {
        uint32_t block_size = SCSI_BLOCK_SIZE;
        bool result =
            scsi->fn.read(scsi->fn.ctx, scsi->read_10.lba, scsi->read_10.count, data, len, cap);
        *len -= *len % block_size;
        uint16_t blocks = *len / block_size;
        scsi->read_10.lba += blocks;
        scsi->read_10.count -= blocks;
        if(!scsi->read_10.count) {
            scsi->tx_done = true;
        }
        return result;
    }; break;
    case SCSI_READ_CD: {
        uint32_t block_size = SCSI_BLOCK_SIZE;
        bool result =
            scsi->fn.read(scsi->fn.ctx, scsi->read_10.lba, scsi->read_10.count, data, len, cap);
        *len -= *len % block_size;
        uint16_t blocks = *len / block_size;
        scsi->read_10.lba += blocks;
        scsi->read_10.count -= blocks;
        if(!scsi->read_10.count) {
            scsi->tx_done = true;
        }
        return result;
    }; break;
    case SCSI_READ_TOC: {
        FURI_LOG_D(TAG, "SCSI_READ_TOC tx");
        if(cap < 2) return false;
        data[0] = 0; // reserved
        data[1] = 10; // MC+DRS: multi class, removable media, session closed
        *len = 2;
        scsi->tx_done = true;
        return true;
    }; break;
    case SCSI_GET_CONFIG: {
        FURI_LOG_D(TAG, "SCSI_GET_CONFIG tx");
        if(cap < 8) return false;
        data[0] = 0; // reserved
        data[1] = 3; // current configuration
        data[2] = 0; // reserved
        data[3] = 0x1C; // additional length: 28 bytes of descriptor data follows
        // Properties field (4 bytes) - bit 0x04 set = read CD/DVD capability
        data[4] = 0;
        data[5] = 0;
        data[6] = 0;
        data[7] = 0x10; // bit 4: read CD/DVD capability
        // Descriptor: descriptive text "CD-RW/DVD-RW Emu"
        uint8_t desc_len = strlen("CD-RW/DVD-RW Emu");
        if(cap < 8 + 2 + desc_len) return false;
        data[8] = 0x14; // descriptor type: descriptive text
        data[9] = (uint8_t)desc_len;
        memcpy(data + 10, "CD-RW/DVD-RW Emu", desc_len);
        *len = 10 + desc_len;
        scsi->tx_done = true;
        return true;
    }; break;
    case SCSI_READ_DISC_INFO: {
        FURI_LOG_D(TAG, "SCSI_READ_DISC_INFO tx");
        if(cap < 8) return false;
        data[0] = 0; // reserved
        data[1] = 0; // disc type: no disc / blank (per MMC spec)
        data[2] = 0x01; // session info: last session opened, multi-session
        data[3] = 0; // reserved
        data[4] = 0; // current LBA high
        data[5] = 0; // current LBA mid
        data[6] = 0; // current LBA low
        data[7] = 0; // current LBA sub
        *len = 8;
        scsi->tx_done = true;
        return true;
    }; break;
    case SCSI_READ_TRACK_INFO: {
        FURI_LOG_D(TAG, "SCSI_READ_TRACK_INFO tx");
        if(cap < 10) return false;
        data[0] = 0; // reserved
        data[1] = 0x01; // number of tracks reported (1 track)
        data[2] = 0; // track number requested (0=all, but we report single)
        data[3] = 0; // reserved
        // Track 0 descriptor: start LBA at 0, next writable area starts at 1
        uint32_t track_start = 0;
        data[4] = (track_start >> 24) & 0xFF;
        data[5] = (track_start >> 16) & 0xFF;
        data[6] = (track_start >> 8) & 0xFF;
        data[7] = track_start & 0xFF;
        // Data format: Mode 1/2 (0x04), address field present
        data[8] = 0x04;
        data[9] = 0x00; // reserved
        *len = 10;
        scsi->tx_done = true;
        return true;
    }; break;
    case SCSI_READ_DVD_STRUCT: {
        FURI_LOG_D(TAG, "SCSI_READ_DVD_STRUCT tx");
        if(cap < 24) return false;
        // DVD-ROM structure page (type 0x00)
        data[0] = 0x00; // structure type: DVD-ROM
        data[1] = 0x07; // length: 7 bytes of descriptor data follow (total 9 bytes per segment)
        data[2] = 0x00; // reserved
        data[3] = 0x00; // reserved
        data[4] = 0x00; // reserved
        data[5] = 0x00; // reserved
        data[6] = 0x00; // reserved
        data[7] = 0x00; // reserved
        data[8] = 0x00; // layer 0: DVD-ROM
        *len = 9;
        scsi->tx_done = true;
        return true;
    }; break;
    default: {
        FURI_LOG_W(TAG, "unexpected scsi tx data cmd=%02X", scsi->cmd[0]);
        scsi->sk = SCSI_SK_ILLEGAL_REQUEST;
        scsi->asc = SCSI_ASC_INVALID_COMMAND_OPERATION_CODE;
        return false;
    }; break;
    }
}

bool scsi_cmd_end(SCSISession* scsi) {
    FURI_LOG_T(TAG, "END %02X", scsi->cmd[0]);
    uint8_t* cmd = scsi->cmd;
    uint8_t len = scsi->cmd_len;
    scsi->cmd = NULL;
    scsi->cmd_len = 0;
    switch(cmd[0]) {
    case SCSI_REQUEST_SENSE:
    case SCSI_INQUIRY:
    case SCSI_READ_FORMAT_CAPACITIES:
    case SCSI_MODE_SENSE_6:
    case SCSI_READ_10:
    case SCSI_READ_12:
    case SCSI_READ_CD:
    case SCSI_READ_TOC:
    case SCSI_GET_CONFIG:
    case SCSI_READ_DISC_INFO:
    case SCSI_READ_TRACK_INFO:
    case SCSI_READ_DVD_STRUCT:
        return scsi->tx_done;

    case SCSI_TEST_UNIT_READY: {
        FURI_LOG_D(TAG, "SCSI_TEST_UNIT_READY");
        return true;
    }; break;
    case SCSI_PREVENT_MEDIUM_REMOVAL: {
        if(len < 6) return false;
        bool prevent = cmd[5];
        FURI_LOG_D(TAG, "SCSI_PREVENT_MEDIUM_REMOVAL prevent=%d", prevent);
        return !prevent;
    }; break;
    case SCSI_START_STOP_UNIT: {
        if(len < 6) return false;
        bool eject = (cmd[4] & 2) != 0;
        bool start = (cmd[4] & 1) != 0;
        FURI_LOG_D(TAG, "SCSI_START_STOP_UNIT eject=%d start=%d", eject, start);
        if(eject) {
            scsi->fn.eject(scsi->fn.ctx);
        }
        return true;
    }; break;
    default: {
        FURI_LOG_W(TAG, "unexpected scsi cmd=%02X", cmd[0]);
        scsi->sk = SCSI_SK_ILLEGAL_REQUEST;
        scsi->asc = SCSI_ASC_INVALID_COMMAND_OPERATION_CODE;
        return false;
    }; break;
    }
}
