#include "Export/JsonWriter.h"

namespace CadImport::App
{
    JsonWriter::JsonWriter() = default;

    void JsonWriter::BeginObject()
    {
        WriteSeparatorIfNeeded();
        m_stream << "{";
        m_needsComma.push_back(false);
    }

    void JsonWriter::EndObject()
    {
        m_stream << "}";
        if (!m_needsComma.empty())
        {
            m_needsComma.pop_back();
        }
        if (!m_needsComma.empty())
        {
            m_needsComma.back() = true;
        }
    }

    void JsonWriter::BeginArray()
    {
        WriteSeparatorIfNeeded();
        m_stream << "[";
        m_needsComma.push_back(false);
    }

    void JsonWriter::EndArray()
    {
        m_stream << "]";
        if (!m_needsComma.empty())
        {
            m_needsComma.pop_back();
        }
        if (!m_needsComma.empty())
        {
            m_needsComma.back() = true;
        }
    }

    void JsonWriter::Key(const std::string& key)
    {
        WriteSeparatorIfNeeded();
        m_stream << "\"" << Escape(key) << "\":";
        m_afterKey = true;
    }

    void JsonWriter::Value(const std::string& value)
    {
        WriteSeparatorIfNeeded();
        m_stream << "\"" << Escape(value) << "\"";
        if (!m_needsComma.empty())
        {
            m_needsComma.back() = true;
        }
    }

    void JsonWriter::Value(const char* value)
    {
        Value(std::string(value ? value : ""));
    }

    void JsonWriter::Value(double value)
    {
        WriteSeparatorIfNeeded();
        m_stream << value;
        if (!m_needsComma.empty())
        {
            m_needsComma.back() = true;
        }
    }

    void JsonWriter::Value(int value)
    {
        WriteSeparatorIfNeeded();
        m_stream << value;
        if (!m_needsComma.empty())
        {
            m_needsComma.back() = true;
        }
    }

    void JsonWriter::Value(bool value)
    {
        WriteSeparatorIfNeeded();
        m_stream << (value ? "true" : "false");
        if (!m_needsComma.empty())
        {
            m_needsComma.back() = true;
        }
    }

    void JsonWriter::ValueNull()
    {
        WriteSeparatorIfNeeded();
        m_stream << "null";
        if (!m_needsComma.empty())
        {
            m_needsComma.back() = true;
        }
    }

    std::string JsonWriter::ToString() const
    {
        return m_stream.str();
    }

    void JsonWriter::WriteSeparatorIfNeeded()
    {
        // A value immediately following Key() never gets a comma or needs
        // to check the container's flag - Key() already handled the
        // member-to-member separator.
        if (m_afterKey)
        {
            m_afterKey = false;
            return;
        }

        if (!m_needsComma.empty() && m_needsComma.back())
        {
            m_stream << ",";
        }
    }

    std::string JsonWriter::Escape(const std::string& input)
    {
        std::string out;
        out.reserve(input.size());
        for (char c : input)
        {
            switch (c)
            {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                out += c;
            }
        }
        return out;
    }
}
