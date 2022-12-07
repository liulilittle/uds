#include <uds/configuration/Ini.h>
#include <uds/collections/Dictionary.h>
#include <uds/io/File.h>

using uds::collections::Dictionary;

namespace uds {
    namespace configuration {
        Ini::Ini() noexcept {

        }

        Ini::Ini(const std::string& config) noexcept {
            std::vector<std::string> lines;
            Tokenize<std::string>(ZTrim(config), lines, "\r\n\r\n");

            std::string sectionKey;
            Section* sectionPtr = NULL;

            for (std::size_t i = 0, length = lines.size(); i < length; i++) {
                std::string& line = lines[i];
                if (line.empty()) {
                    continue;
                }

                std::size_t index = line.find('#');
                if (index != std::string::npos) {
                    if (index == 0) {
                        continue;
                    }
                    line = line.substr(0, index);
                }

                // Not founds sections.
                do {
                    std::size_t leftIndex = line.find('[');
                    if (leftIndex == std::string::npos) {
                        break;
                    }

                    std::size_t rightIndex = line.find(']', leftIndex);
                    if (rightIndex == std::string::npos) {
                        break;
                    }

                    int64_t size = (int64_t)rightIndex - (leftIndex + 1);
                    if (size < 1) {
                        break;
                    }

                    bool correct = true;
                    struct {
                        std::size_t l;
                        std::size_t r;
                    } ranges[2] = { { 0, leftIndex}, {rightIndex + 1, line.size()} };
                    for (std::size_t c = 0; c < 2 && correct; c++) {
                        for (std::size_t j = ranges[c].l; j < ranges[c].r; j++) {
                            char ch = line[j];
                            if (ch != ' ' && 
                                ch != '\t' && 
                                ch != '\r' && 
                                ch != '\n' &&
                                ch != '\0') {
                                correct = false;
                                break;
                            }
                        }
                    }

                    if (correct) {
                        sectionKey = RTrim(LTrim(line.substr(leftIndex + 1, size)));
                        if (sectionKey.empty()) {
                            continue;
                        }

                        sectionPtr = Add(sectionKey);
                        if (!sectionPtr) { /* The configuration file format is problematic. */
                            sectionPtr = Get(sectionKey);
                        }
                    }
                } while (0);

                // Read section all key-values.
                if (sectionKey.size()) { 
                    index = line.find('=');
                    if (index == std::string::npos) {
                        index = line.find(':');
                        if (index == std::string::npos) {
                            continue;
                        }
                    }

                    if (index == 0) {
                        continue;
                    }

                    std::string key = RTrim(LTrim(line.substr(0, index)));
                    std::string value = RTrim(LTrim(line.substr(index + 1)));
                    sectionPtr->SetValue(key, value);
                }
            }
        }

        Ini::Section& Ini::operator[](const std::string& section) {
            if (section.empty()) {
                throw std::invalid_argument("section cannot be an empty string.");
            }

            Section* p = Get(section);
            if (p) {
                return *p;
            }

            p = Add(section);
            if (p) {
                return *p;
            }
            throw std::runtime_error("unable to complete adding new section.");
        }

        Ini::Section* Ini::Get(const std::string& section) noexcept {
            if (section.empty()) {
                return NULL;
            }

            Ini::Section* out = NULL;
            Dictionary::TryGetValuePointer(sections_, section, out);
            return out;
        }

        Ini::Section* Ini::Add(const std::string& section) noexcept {
            if (section.empty()) {
                return NULL;
            }

            if (Dictionary::ContainsKey(sections_, section)) {
                return NULL;
            }

            SectionTable::iterator indexer;
            if (!Dictionary::TryAdd(sections_, section, Ini::Section(section), indexer)) {
                return NULL;
            }
            return addressof(indexer->second);
        }

        bool Ini::Remove(const std::string& section) noexcept {
            if (section.empty()) {
                return false;
            }

            return Dictionary::TryRemove(sections_, section);
        }

        bool Ini::ContainsKey(const std::string& section) noexcept {
            return NULL != Get(section);
        }

        int Ini::Count() noexcept {
            return sections_.size();
        }

        int Ini::GetAllKeys(std::vector<std::string>& keys) noexcept {
            return Dictionary::GetAllKeys(sections_, keys);
        }

        int Ini::GetAllPairs(std::vector<std::pair<const std::string&, const Section&> >& pairs) noexcept {
            return Dictionary::GetAllPairs(sections_, pairs);
        }

        std::string Ini::ToString() const noexcept {
            SectionTable::iterator tail = sections_.begin();
            SectionTable::iterator endl = sections_.end();

            std::string config;
            for (; tail != endl; tail++) {
                std::string section = tail->second.ToString();
                if (section.empty()) {
                    continue;
                }

                if (config.empty()) {
                    config.append(section);
                }
                else {
                    config.append("\r\n\r\n" + section);
                }
            }
            return config;
        }

        Ini::iterator Ini::begin() noexcept {
            return sections_.begin();
        }

        Ini::iterator Ini::end() noexcept {
            return sections_.end();
        }

        Ini::Section::Section(const std::string& name) noexcept
            : Name(name) {
            
        }

        std::string& Ini::Section::operator[](const std::string& key) {
            if (key.empty()) {
                throw std::invalid_argument("key cannot be an empty string.");
            }
            return kv_[key];
        }

        bool Ini::Section::RemoveValue(const std::string& key) noexcept {
            if (key.empty()) {
                return false;
            }

            return Dictionary::TryRemove(kv_, key);
        }

        template<>
        std::string Ini::Section::GetValue(const std::string& key) noexcept {
            std::string value = GetValue(key);
            return value;
        }

        template<>
        int32_t Ini::Section::GetValue(const std::string& key) noexcept {
            std::string value = GetValue(key);
            return strtol(value.data(), NULL, 10);
        }

        template<>
        uint32_t Ini::Section::GetValue(const std::string& key) noexcept {
            std::string value = GetValue(key);
            return strtoul(value.data(), NULL, 10);
        }

        template<>
        int64_t Ini::Section::GetValue(const std::string& key) noexcept {
            std::string value = GetValue(key);
            return strtoll(value.data(), NULL, 10);
        }

        template<>
        uint64_t Ini::Section::GetValue(const std::string& key) noexcept {
            std::string value = GetValue(key);
            return strtoull(value.data(), NULL, 10);
        }

        template<>
        float Ini::Section::GetValue(const std::string& key) noexcept {
            std::string value = GetValue(key);
            return strtof(value.data(), NULL);
        }

        template<>
        double Ini::Section::GetValue(const std::string& key) noexcept {
            std::string value = GetValue(key);
            return strtod(value.data(), NULL);
        }

        template<>
        bool Ini::Section::GetValue(const std::string& key) noexcept {
            std::string value = GetValue(key);
            return ToBoolean(value.data());
        }

        std::string Ini::Section::GetValue(const std::string& key) noexcept {
            if (key.empty()) {
                return std::string();
            }

            std::string value;
            Dictionary::TryGetValue(kv_, key, value);
            return std::move(value);
        }

        int Ini::Section::Count() noexcept {
            return kv_.size();
        }

        bool Ini::Section::ContainsKey(const std::string& key) noexcept {
            if (key.empty()) {
                return false;
            }

            return Dictionary::ContainsKey(kv_, key);
        }

        bool Ini::Section::SetValue(const std::string& key, const std::string& value) noexcept {
            if (key.empty()) {
                return false;
            }

            Section& section = *this;
            section[key] = value;
            return true;
        }

        int Ini::Section::GetAllKeys(std::vector<std::string>& keys) noexcept {
            return Dictionary::GetAllKeys(kv_, keys);
        }

        int Ini::Section::GetAllPairs(std::vector<std::pair<const std::string&, const std::string&> >& pairs) noexcept {
            return Dictionary::GetAllPairs(kv_, pairs);
        }

        std::string Ini::Section::ToString() const noexcept {
            KeyValueTable::iterator tail = kv_.begin();
            KeyValueTable::iterator endl = kv_.end();
            if (tail == endl) {
                return std::string();
            }

            std::string result;
            result.append("[");
            result.append(Name);
            result.append("]\r\n");
            for (; tail != endl; tail++) {
                result.append(tail->first + "=" + tail->second + "\r\n");
            }
            return result.substr(0, result.size() - 2);
        }

        Ini::Section::iterator Ini::Section::begin() noexcept {
            return kv_.begin();
        }

        Ini::Section::iterator Ini::Section::end() noexcept {
            return kv_.end();
        }

        std::shared_ptr<Ini> Ini::LoadFile(const std::string& path) noexcept {
            if (path.empty()) {
                return make_shared_object<Ini>();
            }

            int length = path.size();
            std::shared_ptr<Byte> config = uds::io::File::ReadAllBytes(path.data(), length);

            if (NULL == config || length < 1) {
                return make_shared_object<Ini>();
            }
            return make_shared_object<Ini>((char*)config.get(), length);
        }

        std::shared_ptr<Ini> Ini::LoadFrom(const std::string& config) noexcept {
            if (config.empty()) {
                return make_shared_object<Ini>();
            }
            return make_shared_object<Ini>(config);
        }
    }
}