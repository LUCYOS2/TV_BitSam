#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace CadImport::App
{
    // Minimal, dependency-free JSON writer purpose-built for the fixed
    // Scene Manifest schema this module exports. Not a general-purpose JSON
    // library and not a parser - if the project later needs to *read* JSON
    // (e.g. importing ROI config), prefer swapping in a real library
    // (nlohmann/json) behind JsonSceneExporter's interface rather than
    // extending this class.
    class JsonWriter
    {
    public:
        JsonWriter();

        void BeginObject();
        void EndObject();
        void BeginArray();
        void EndArray();

        // Writes `"key":` - call before a Begin*/Write* value call.
        void Key(const std::string& key);

        void Value(const std::string& value);
        void Value(const char* value);
        void Value(double value);
        void Value(int value);
        void Value(bool value);
        void ValueNull();

        std::string ToString() const;

    private:
        void WriteSeparatorIfNeeded();
        static std::string Escape(const std::string& input);

        std::ostringstream m_stream;
        // one bool per open container: true once a member has been written,
        // so the next member/element knows to prefix a comma.
        std::vector<bool> m_needsComma;
        // true right after Key() until the corresponding value is written -
        // suppresses the separator check for that one value, since Key()
        // already emitted the member separator.
        bool m_afterKey = false;
    };
}
