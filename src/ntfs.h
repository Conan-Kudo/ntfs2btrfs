/* Copyright (c) Mark Harmstone 2020
 *
 * This file is part of ntfs2btrfs.
 *
 * Ntfs2btrfs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public Licence as published by
 * the Free Software Foundation, either version 2 of the Licence, or
 * (at your option) any later version.
 *
 * Ntfs2btrfs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public Licence
 * along with Ntfs2btrfs. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include <stdint.h>
#include <fstream>
#include <vector>
#include <string>
#include <list>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#endif

#pragma pack(push,1)

typedef struct {
    uint8_t Jmp[3];
    uint8_t FsName[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t Unused1[5];
    uint8_t Media;
    uint8_t Unused2[2];
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t Unused3;
    uint32_t Unknown;
    uint64_t TotalSectors;
    uint64_t MFT;
    uint64_t MFTMirr;
    int8_t ClustersPerMFTRecord;
    uint8_t Padding1[3];
    int8_t ClustersPerIndexRecord;
    uint8_t Padding2[3];
    uint64_t SerialNumber;
    uint32_t Checksum;
} NTFS_BOOT_SECTOR;

#define NTFS_FS_NAME "NTFS    "

// https://docs.microsoft.com/en-us/windows/win32/devnotes/attribute-record-header
#define ATTRIBUTE_FLAG_COMPRESSION_MASK 0x00ff
#define ATTRIBUTE_FLAG_SPARSE 0x8000
#define ATTRIBUTE_FLAG_ENCRYPTED 0x4000

enum class NTFS_ATTRIBUTE_FORM : uint8_t {
    RESIDENT_FORM = 0,
    NONRESIDENT_FORM = 1
};

enum class ntfs_attribute : uint32_t {
    STANDARD_INFORMATION = 0x10,
    ATTRIBUTE_LIST = 0x20,
    FILE_NAME = 0x30,
    VOLUME_VERSION = 0x40,
    SECURITY_DESCRIPTOR = 0x50,
    VOLUME_NAME = 0x60,
    VOLUME_INFORMATION = 0x70,
    DATA = 0x80,
    INDEX_ROOT = 0x90,
    INDEX_ALLOCATION = 0xA0,
    BITMAP = 0xB0,
    SYMBOLIC_LINK = 0xC0,
    EA_INFORMATION = 0xD0,
    EA = 0xE0,
    PROPERTY_SET = 0xF0,
};

typedef struct {
    enum ntfs_attribute TypeCode;
    uint16_t RecordLength;
    uint16_t Unknown;
    NTFS_ATTRIBUTE_FORM FormCode;
    uint8_t NameLength;
    uint16_t NameOffset;
    uint16_t Flags;
    uint16_t Instance;
    union {
        struct {
            uint32_t ValueLength;
            uint16_t ValueOffset;
            uint8_t Reserved[2];
        } Resident;
        struct {
            uint64_t LowestVcn;
            uint64_t HighestVcn;
            uint16_t MappingPairsOffset;
            uint16_t CompressionUnit;
            uint32_t Padding;
            uint64_t AllocatedLength;
            uint64_t FileSize;
            uint64_t ValidDataLength;
            uint64_t TotalAllocated;
        } Nonresident;
    } Form;
} ATTRIBUTE_RECORD_HEADER;

// https://docs.microsoft.com/en-us/windows/win32/devnotes/multi-sector-header
typedef struct {
    uint32_t Signature;
    uint16_t UpdateSequenceArrayOffset;
    uint16_t UpdateSequenceArraySize;
} MULTI_SECTOR_HEADER;

// https://docs.microsoft.com/en-us/windows/win32/devnotes/mft-segment-reference
typedef struct {
    uint64_t SegmentNumber : 48;
    uint64_t SequenceNumber : 16;
} MFT_SEGMENT_REFERENCE;

// based on https://docs.microsoft.com/en-us/windows/win32/devnotes/file-record-segment-header and
// http://www.cse.scu.edu/~tschwarz/coen252_07Fall/Lectures/NTFS.html
typedef struct {
    MULTI_SECTOR_HEADER MultiSectorHeader;
    uint64_t LogFileSequenceNumber;
    uint16_t SequenceNumber;
    uint16_t HardLinkCount;
    uint16_t FirstAttributeOffset;
    uint16_t Flags;
    uint32_t EntryUsedSize;
    uint32_t EntryAllocatedSize;
    MFT_SEGMENT_REFERENCE BaseFileRecordSegment;
    uint16_t NextAttributeID;
} FILE_RECORD_SEGMENT_HEADER;

#define FILE_RECORD_SEGMENT_IN_USE      1
#define FILE_RECORD_IS_DIRECTORY        2

static const uint32_t NTFS_FILE_SIGNATURE = 0x454c4946; // "FILE"

#define NTFS_VOLUME_INODE       3
#define NTFS_ROOT_DIR_INODE     5
#define NTFS_BITMAP_INODE       6
#define NTFS_SECURE_INODE       9

// https://flatcap.org/linux-ntfs/ntfs/attributes/standard_information.html

typedef struct {
    int64_t CreationTime;
    int64_t LastAccessTime;
    int64_t LastWriteTime;
    int64_t ChangeTime;
    uint32_t FileAttributes;
    uint32_t MaximumVersions;
    uint32_t VersionNumber;
    uint32_t ClassId;
    uint32_t OwnerId;
    uint32_t SecurityId;
    uint32_t QuotaCharged;
    uint32_t USN;
} STANDARD_INFORMATION;

#define FILE_ATTRIBUTE_READONLY             0x00000001
#define FILE_ATTRIBUTE_HIDDEN               0x00000002
#define FILE_ATTRIBUTE_SYSTEM               0x00000004
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020
#define FILE_ATTRIBUTE_DEVICE               0x00000040
#define FILE_ATTRIBUTE_NORMAL               0x00000080
#define FILE_ATTRIBUTE_TEMPORARY            0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400
#define FILE_ATTRIBUTE_COMPRESSED           0x00000800
#define FILE_ATTRIBUTE_OFFLINE              0x00001000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000
#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000
#define FILE_ATTRIBUTE_VIRTUAL              0x00010000

#define FILE_ATTRIBUTE_DIRECTORY_MFT        0x10000000

// https://flatcap.org/linux-ntfs/ntfs/attributes/file_name.html

typedef struct {
    MFT_SEGMENT_REFERENCE Parent;
    int64_t CreationTime;
    int64_t LastAccessTime;
    int64_t LastWriteTime;
    int64_t ChangeTime;
    uint64_t AllocationSize;
    uint64_t EndOfFile;
    uint32_t FileAttributes;
    uint32_t EaSize;
    uint8_t FileNameLength;
    uint8_t Namespace;
    char16_t FileName[1];
} FILE_NAME;

#define FILE_NAME_POSIX         0
#define FILE_NAME_WIN32         1
#define FILE_NAME_DOS           2
#define FILE_NAME_WIN32_AND_DOS 3

// https://flatcap.org/linux-ntfs/ntfs/concepts/node_header.html

typedef struct {
    uint32_t first_entry;
    uint32_t total_size;
    uint32_t allocated_size;
    uint32_t flags;
} index_node_header;

// https://flatcap.org/linux-ntfs/ntfs/concepts/index_entry.html

#define INDEX_ENTRY_SUBNODE     1
#define INDEX_ENTRY_LAST        2

typedef struct {
    MFT_SEGMENT_REFERENCE file_reference;
    uint16_t entry_length;
    uint16_t stream_length;
    uint32_t flags;
} index_entry;

// https://flatcap.org/linux-ntfs/ntfs/attributes/index_root.html

typedef struct {
    uint32_t attribute_type;
    uint32_t collation_rule;
    uint32_t bytes_per_index_record;
    uint8_t clusters_per_index_record;
    uint8_t padding[3];
    index_node_header node_header;
    index_entry entries[1];
} index_root;

// https://flatcap.org/linux-ntfs/ntfs/concepts/index_record.html

typedef struct {
    MULTI_SECTOR_HEADER MultiSectorHeader;
    uint64_t sequence_number;
    uint64_t vcn;
    index_node_header header;
    uint16_t update_sequence;
} index_record;

#define INDEX_RECORD_MAGIC 0x58444e49 // "INDX"

// https://flatcap.org/linux-ntfs/ntfs/files/secure.html

typedef struct {
    uint32_t hash;
    uint32_t id;
    uint64_t offset;
    uint32_t length;
} sd_entry;

// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_reparse_data_buffer

typedef struct {
    uint32_t ReparseTag;
    uint16_t ReparseDataLength;
    uint16_t Reserved;

    union {
        struct {
            uint16_t SubstituteNameOffset;
            uint16_t SubstituteNameLength;
            uint16_t PrintNameOffset;
            uint16_t PrintNameLength;
            uint32_t Flags;
            char16_t PathBuffer[1];
        } SymbolicLinkReparseBuffer;

        struct {
            uint16_t SubstituteNameOffset;
            uint16_t SubstituteNameLength;
            uint16_t PrintNameOffset;
            uint16_t PrintNameLength;
            char16_t PathBuffer[1];
        } MountPointReparseBuffer;

        struct {
            uint8_t DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER;

#ifndef IO_REPARSE_TAG_SYMLINK
#define IO_REPARSE_TAG_SYMLINK      0xa000000c
#endif

#define IO_REPARSE_TAG_LX_SYMLINK   0xa000001d
#define IO_REPARSE_TAG_WOF          0x80000017

#ifndef SYMLINK_FLAG_RELATIVE
#define SYMLINK_FLAG_RELATIVE       0x00000001
#endif

// https://flatcap.org/linux-ntfs/ntfs/attributes/volume_information.html

typedef struct {
    uint64_t Unknown1;
    uint8_t MajorVersion;
    uint8_t MinorVersion;
    uint16_t Flags;
    uint32_t Unknown2;
} VOLUME_INFORMATION;

#define NTFS_VOLUME_DIRTY               0x0001
#define NTFS_VOLUME_RESIZE_JOURNAL      0x0002
#define NTFS_VOLUME_UPGRADE_ON_MOUNT    0x0004
#define NTFS_VOLUME_MOUNTED_ON_NT4      0x0008
#define NTFS_VOLUME_DELETE_USN_UNDERWAY 0x0010
#define NTFS_VOLUME_REPAIR_OBJECT_IDS   0x0020
#define NTFS_VOLUME_MODIFIED_BY_CHKDSK  0x8000

// https://flatcap.org/linux-ntfs/ntfs/attributes/attribute_list.html

typedef struct {
    enum ntfs_attribute type;
    uint16_t record_length;
    uint8_t name_length;
    uint8_t name_offset;
    uint64_t starting_vcn;
    MFT_SEGMENT_REFERENCE file_reference;
    uint16_t instance;
} attribute_list_entry;

#define WOF_CURRENT_VERSION         1

#define WOF_PROVIDER_WIM            1
#define WOF_PROVIDER_FILE           2

typedef struct {
    uint32_t ReparseTag;
    uint16_t ReparseDataLength;
    uint16_t Reserved;
    uint8_t DataBuffer[1];
} reparse_point_header; // edited form of REPARSE_DATA_BUFFER

typedef struct {
    uint32_t Version;
    uint32_t Provider;
} wof_external_info; // WOF_EXTERNAL_INFO in winioctl.h

#define FILE_PROVIDER_CURRENT_VERSION           1

#define FILE_PROVIDER_COMPRESSION_XPRESS4K          0
#define FILE_PROVIDER_COMPRESSION_LZX               1
#define FILE_PROVIDER_COMPRESSION_XPRESS8K          2
#define FILE_PROVIDER_COMPRESSION_XPRESS16K         3

typedef struct {
    uint32_t Version;
    uint32_t Algorithm;
} file_provider_external_info_v0; // FILE_PROVIDER_EXTERNAL_INFO_V0 in winioctl.h

#pragma pack(pop)

class ntfs;

struct mapping {
    mapping(uint64_t lcn, uint64_t vcn, uint64_t length) : lcn(lcn), vcn(vcn), length(length) { }

    uint64_t lcn;
    uint64_t vcn;
    uint64_t length;
};

class ntfs_file {
public:
    ntfs_file(ntfs& dev, uint64_t inode);
    std::string read(size_t offset = 0, size_t length = 0, enum ntfs_attribute type = ntfs_attribute::DATA, const std::u16string_view& name = u"");
    std::list<mapping> read_mappings(enum ntfs_attribute type = ntfs_attribute::DATA, const std::u16string_view& name = u"");

    bool is_directory() const {
        return file_record->Flags & FILE_RECORD_IS_DIRECTORY;
    }

    void loop_through_atts(const std::function<bool(const ATTRIBUTE_RECORD_HEADER*, const std::string_view&, const std::u16string_view&)>& func);
    std::string get_filename();

    FILE_RECORD_SEGMENT_HEADER* file_record;

private:
    std::string read_nonresident_attribute(size_t offset, size_t length, const ATTRIBUTE_RECORD_HEADER* att);

    std::vector<char> file_record_buf;
    ntfs& dev;
    uint64_t inode;
};

class ntfs {
public:
    ntfs(const std::string& fn);

    ~ntfs() {
        if (mft)
            delete mft;

#ifdef _WIN32
        CloseHandle(h);
#endif
    }

    void seek(size_t pos);
    void read(char* buf, size_t length);
    void write(const char* buf, size_t length);

    ntfs_file* mft = nullptr;
    std::vector<char> boot_sector_buf;
    NTFS_BOOT_SECTOR* boot_sector;
    uint64_t file_record_size;

private:
#ifdef _WIN32
    HANDLE h;
#else
    std::fstream f;
#endif
};
