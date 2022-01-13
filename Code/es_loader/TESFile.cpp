#include "TESFile.h"

#include <filesystem>
#include <fstream>

#include "Records/Record.h"
#include "Records/REFR.h"

TESFile::TESFile(const std::filesystem::path& acPath)
{
    m_filename = acPath.filename().string();

    const uintmax_t fileSize = std::filesystem::file_size(acPath);
    m_buffer.Resize(fileSize);

    std::ifstream file(acPath, std::ios::binary);
    file.read(reinterpret_cast<char*>(m_buffer.GetWriteData()), fileSize);

    BuildFormIdRecordMap();
}

void TESFile::BuildFormIdRecordMap() noexcept
{
    Buffer::Reader reader(&m_buffer);

    while (true)
    {
        if (!ReadGroupOrRecord(reader))
            break;
    }
}

bool TESFile::ReadGroupOrRecord(Buffer::Reader& aReader) noexcept
{
    if (aReader.Eof())
        return false;

    uint32_t type = 0;
    aReader.ReadBytes(reinterpret_cast<uint8_t*>(&type), 4);
    uint32_t size = 0;
    aReader.ReadBytes(reinterpret_cast<uint8_t*>(&size), 4);

    if (type == static_cast<uint32_t>(FormEnum::GRUP))
    {
        Group* pGroup = reinterpret_cast<Group*>(m_buffer.GetWriteData() + aReader.GetBytePosition());
        GroupData data;

        aReader.Advance(sizeof(Group));
        const size_t endOfGroup = aReader.GetBytePosition() + size - 0x18;

        while (aReader.GetBytePosition() < endOfGroup)
        {
            const uint8_t* subGroup = m_buffer.GetData() + aReader.GetBytePosition();
            if (ReadGroupOrRecord(aReader))
            {
                data.m_subGroups.push_back(subGroup);
            }
        }

        m_groupDataMap[pGroup] = data;
    }
    else // Records
    {
        Record* record = reinterpret_cast<Record*>(m_buffer.GetWriteData() + aReader.GetBytePosition());
        m_formIdRecordMap[record->GetFormId()] = record;

        switch (record->GetType())
        {
        case FormEnum::REFR:
        //case FormEnum::ACHR:
            REFR* recordREFR = reinterpret_cast<REFR*>(record);
            m_objectReferences[record->GetFormId()] = recordREFR;
            break;
        case FormEnum::CELL:
            m_cells.push_back(record);
            break;
        }

        aReader.Advance(sizeof(Record) + size);
    }

    return true;
}

template<class T>
Vector<T> TESFile::GetRecords() noexcept
{
    Buffer::Reader reader(&m_buffer);
}

