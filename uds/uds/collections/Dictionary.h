#pragma once

#include <uds/stdafx.h>

namespace uds {
    namespace collections {
        class Dictionary final {
        public:
            template<typename TDictionary, typename TKey>
            inline static int GetAllKeys(TDictionary& dictionary, std::vector<TKey>& keys) noexcept {
                typename TDictionary::iterator tail = dictionary.begin();
                typename TDictionary::iterator endl = dictionary.end();

                int length = 0;
                for (; tail != endl; tail++) {
                    length++;
                    keys.push_back(tail->first);
                }
                return length;
            }

            template<typename TDictionary, typename TKey, typename TValue>
            inline static int GetAllPairs(TDictionary& dictionary, std::vector<std::pair<const TKey&, const TValue&> >& keys) noexcept {
                typename TDictionary::iterator tail = dictionary.begin();
                typename TDictionary::iterator endl = dictionary.end();

                int length = 0;
                for (; tail != endl; tail++) {
                    length++;
                    keys.push_back(std::make_pair(tail->first, tail->second));
                }
                return length;
            }
        
            template<typename TDictionary, typename CloseHandler>
            inline static int ReleaseAllPairs(TDictionary& dictionary, CloseHandler&& handler) noexcept {
                typename TDictionary::iterator tail = dictionary.begin();
                typename TDictionary::iterator endl = dictionary.end();

                typedef typename TDictionary::value_type TKeyValuePair;
                typedef typename TKeyValuePair::second_type TValue;

                std::vector<TValue> releases;
                if (dictionary.size()) {
                    typename TDictionary::iterator tail = dictionary.begin();
                    typename TDictionary::iterator endl = dictionary.end();
                    for (; tail != endl; tail++) {
                        releases.push_back(std::move(tail->second));
                    }
                    dictionary.clear();
                }

                std::size_t length = releases.size();
                for (std::size_t index = 0; index < length; index++) {
                    TValue p = std::move(releases[index]);
                    handler(p);
                }
                return length;
            }

            template<typename TDictionary>
            inline static int ReleaseAllPairs(TDictionary& dictionary) noexcept {
                typedef typename TDictionary::value_type TKeyValuePair;
                typedef typename TKeyValuePair::second_type TValue;

                return ReleaseAllPairs(dictionary,
                    [](TValue& p) noexcept {
                        p->Close();
                    });
            }

            template<typename TDictionary, typename CloseHandler>
            inline static int ReleaseAllPairs2Layer(TDictionary& dictionary, CloseHandler&& handler) noexcept {
                typename TDictionary::iterator tail = dictionary.begin();
                typename TDictionary::iterator endl = dictionary.end();

                typedef typename TDictionary::value_type::second_type TSubDictionary;
                typedef typename TSubDictionary::value_type TKeyValuePair;
                typedef typename TKeyValuePair::second_type TValue;

                std::vector<TValue> releases;
                if (dictionary.size()) {
                    typename TDictionary::iterator tail = dictionary.begin();
                    typename TDictionary::iterator endl = dictionary.end();
                    for (; tail != endl; tail++) {
                        TSubDictionary& subdictionary = tail->second;
                        typename TSubDictionary::iterator tail2 = subdictionary.begin();
                        typename TSubDictionary::iterator endl2 = subdictionary.end();
                        for (; tail2 != endl2; tail2++) {
                            releases.push_back(std::move(tail2->second));
                        }
                        subdictionary.clear();
                    }
                    dictionary.clear();
                }

                std::size_t length = releases.size();
                for (std::size_t index = 0; index < length; index++) {
                    TValue p = std::move(releases[index]);
                    handler(p);
                }
                return length;
            }

            template<typename TDictionary>
            inline static int ReleaseAllPairs2Layer(TDictionary& dictionary) noexcept {
                typedef typename TDictionary::value_type::second_type TSubDictionary;
                typedef typename TSubDictionary::value_type TKeyValuePair;
                typedef typename TKeyValuePair::second_type TValue;

                return ReleaseAllPairs2Layer(dictionary, 
                    [](TValue& p) noexcept {
                        p->Close();
                    });
            }

            template<typename TDictionary, typename TKey>
            inline static bool TryRemove(TDictionary& dictionary, const TKey& key) noexcept {
                typename TDictionary::iterator tail = dictionary.find(key);
                typename TDictionary::iterator endl = dictionary.end();
                if (tail == endl) {
                    return false;
                }

                dictionary.erase(tail);
                return true;
            }

            template<typename TDictionary, typename TKey, typename TValue>
            inline static bool TryRemove(TDictionary& dictionary, const TKey& key, TValue& value) noexcept {
                typename TDictionary::iterator tail = dictionary.find(key);
                typename TDictionary::iterator endl = dictionary.end();
                if (tail == endl) {
                    return false;
                }

                value = std::move(tail->second);
                dictionary.erase(tail);
                return true;
            }

            template<typename TDictionary, typename TKey1, typename TKey2>
            inline static bool TryRemove2Layer(TDictionary& dictionary, const TKey1& key1, const TKey2& key2) noexcept {
                typedef typename TDictionary::value_type::second_type TSubDictionary;

                TSubDictionary* subdictionary = NULL;
                if (!Dictionary::TryGetValuePointer(dictionary, key1, subdictionary)) {
                    return false;
                }
                elif(!Dictionary::TryRemove(*subdictionary, key2)) {
                    return false;
                }
                elif(!subdictionary->empty()) {
                    return true;
                }
                return Dictionary::TryRemove(dictionary, key1);
            }

            template<typename TDictionary, typename TKey1, typename TKey2, typename TValue>
            inline static bool TryRemove2Layer(TDictionary& dictionary, const TKey1& key1, const TKey2& key2, TValue& value) noexcept {
                typedef typename TDictionary::value_type::second_type TSubDictionary;

                TSubDictionary* subdictionary = NULL;
                if (!Dictionary::TryGetValuePointer(dictionary, key1, subdictionary)) {
                    return false;
                }
                elif(!Dictionary::TryRemove(*subdictionary, key2, value)) {
                    return false;
                }
                elif(!subdictionary->empty()) {
                    return true;
                }
                return Dictionary::TryRemove(dictionary, key1);
            }

            template<typename TDictionary, typename TKey1, typename TKey2, typename TValue>
            inline static bool TryGetValuePointer2Layer(TDictionary& dictionary, const TKey1& key1, const TKey2& key2, TValue*& value) noexcept {
                typedef typename TDictionary::value_type::second_type TSubDictionary;

                TSubDictionary* subdictionary = NULL;
                if (!Dictionary::TryGetValuePointer(dictionary, key1, subdictionary)) {
                    return false;
                }
                return Dictionary::TryGetValuePointer(*subdictionary, key2, value);
            }

            template<typename TDictionary, typename TKey, typename TValue>
            inline static bool TryGetValuePointer(TDictionary& dictionary, const TKey& key, TValue*& value) noexcept {
                typename TDictionary::iterator tail = dictionary.find(key);
                typename TDictionary::iterator endl = dictionary.end();
                if (tail == endl) {
                    value = NULL;
                    return false;
                }

                value = addressof(tail->second);
                return true;
            }

            template<typename TDictionary, typename TKey1, typename TKey2, typename TValue>
            inline static bool TryGetValue2Layer(TDictionary& dictionary, const TKey1& key1, const TKey2& key2, TValue& value) noexcept {
                typedef typename TDictionary::value_type::second_type TSubDictionary;

                TSubDictionary* subdictionary = NULL;
                if (!Dictionary::TryGetValuePointer(dictionary, key1, subdictionary)) {
                    return false;
                }
                return Dictionary::TryGetValue(*subdictionary, key2, value);
            }

            template<typename TDictionary, typename TKey, typename TValue>
            inline static bool TryGetValue(TDictionary& dictionary, const TKey& key, TValue& value) noexcept {
                TValue* out = NULL;
                if (!TryGetValuePointer(dictionary, key, out)) {
                    return false;
                }

                value = *out;
                return true;
            }

            template<typename TDictionary, typename TKey>
            inline static bool ContainsKey(TDictionary& dictionary, const TKey& key) noexcept {
                typename TDictionary::iterator tail = dictionary.find(key);
                typename TDictionary::iterator endl = dictionary.end();
                return tail != endl;
            }

            template<typename TDictionary, typename TKey, typename TValue>
            inline static bool TryAdd(TDictionary& dictionary, const TKey& key, const TValue& value, typename TDictionary::iterator& indexer) noexcept {
                std::pair<typename TDictionary::iterator, bool> pair = dictionary.insert(std::make_pair(key, value));
                indexer = pair.first;
                return pair.second;
            }

            template<typename TDictionary, typename TKey, typename TValue>
            inline static bool TryAdd(TDictionary& dictionary, const TKey& key, const TValue& value) noexcept {
                return dictionary.insert(std::make_pair(key, value)).second;
            }

            template<typename TDictionary, typename TKey, typename TValue>
            inline static bool TryAdd(TDictionary& dictionary, const TKey& key, TValue&& value) noexcept {
                return dictionary.insert(std::make_pair(key, value)).second;
            }

            template<typename TDictionary, typename TKey, typename TValue>
            inline static bool TryAdd(TDictionary& dictionary, TKey&& key, TValue&& value) noexcept {
                return dictionary.insert(std::make_pair(key, value)).second;
            }

            template<typename TKey, typename TDictionary>
            inline static TKey Min(TDictionary& dictionary, const TKey& defaultKey) noexcept {
                typename TDictionary::iterator tail = dictionary.begin();
                typename TDictionary::iterator endl = dictionary.end();
                if (tail == endl) {
                    return defaultKey;
                }

                TKey key = defaultKey;
                std::size_t key_size = 0;

                for (; tail != endl; tail++) {
                    std::size_t nxt_size = tail->second;
                    if (!key || key_size > nxt_size) {
                        key = tail->first;
                        key_size = nxt_size;
                    }
                }
                return key;
            }

            template<typename TKey, typename TDictionary>
            inline static TKey Max(TDictionary& dictionary, const TKey& defaultKey) noexcept {
                typename TDictionary::iterator tail = dictionary.begin();
                typename TDictionary::iterator endl = dictionary.end();
                if (tail == endl) {
                    return defaultKey;
                }

                TKey key = defaultKey;
                std::size_t key_size = 0;

                for (; tail != endl; tail++) {
                    std::size_t nxt_size = tail->second;
                    if (!key || key_size < nxt_size) {
                        key = tail->first;
                        key_size = nxt_size;
                    }
                }
                return key;
            }
        };
    }
}