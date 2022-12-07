#pragma once

#include <uds/stdafx.h>

namespace uds {
    namespace configuration {
        class Ini final {
        public:
            class Section final {
            public:
                typedef std::map<std::string, std::string>              KeyValueTable;
                typedef KeyValueTable::iterator                         iterator;

            public:
                const std::string                                       Name;

            public:
                Section(const std::string& name) noexcept;

            public:
                std::string&                                            operator[](const std::string& key);

            public:
                iterator                                                begin() noexcept;
                iterator                                                end() noexcept;

            public:
                template<typename TValue>
                TValue                                                  GetValue(const std::string& key) noexcept;
                std::string                                             GetValue(const std::string& key) noexcept;

            public:
                int                                                     Count() noexcept;
                bool                                                    ContainsKey(const std::string& key) noexcept;
                bool                                                    RemoveValue(const std::string& key) noexcept;
                bool                                                    SetValue(const std::string& key, const std::string& value) noexcept;
                int                                                     GetAllKeys(std::vector<std::string>& keys) noexcept;
                int                                                     GetAllPairs(std::vector<std::pair<const std::string&, const std::string&> >& pairs) noexcept;
                std::string                                             ToString() const noexcept;

            private:
                mutable KeyValueTable                                   kv_;
            };
            typedef std::map<std::string, Section>                      SectionTable;
            typedef SectionTable::iterator                              iterator;

        public:
            inline Ini(const void* config) noexcept
                : Ini(config ? std::string((char*)config) : std::string()) {

            }
            inline Ini(const void* config, int length) noexcept
                : Ini(config&& length > 0 ? std::string((char*)config, length) : std::string()) {

            }
            Ini() noexcept;
            Ini(const std::string& config) noexcept;

        public:
            iterator                                                    begin() noexcept;
            iterator                                                    end() noexcept;

        public:
            Section&                                                    operator[](const std::string& section);

        public:
            int                                                         Count() noexcept;
            Section*                                                    Get(const std::string& section) noexcept;
            Section*                                                    Add(const std::string& section) noexcept;
            bool                                                        Remove(const std::string& section) noexcept;
            bool                                                        ContainsKey(const std::string& section) noexcept;
            int                                                         GetAllKeys(std::vector<std::string>& keys) noexcept;
            int                                                         GetAllPairs(std::vector<std::pair<const std::string&, const Section&> >& pairs) noexcept;
            std::string                                                 ToString() const noexcept;

        public:
            static std::shared_ptr<Ini>                                 LoadFile(const std::string& path) noexcept;
            static std::shared_ptr<Ini>                                 LoadFrom(const std::string& config) noexcept;

        private:
            mutable SectionTable                                        sections_;
        };
    }
}