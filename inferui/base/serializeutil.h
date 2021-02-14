/*
   Copyright 2018 Software Reliability Lab, ETH Zurich

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef CC_SYNTHESIS_SERIALIZEUTIL_H
#define CC_SYNTHESIS_SERIALIZEUTIL_H

#include <cstdio>
#include <glog/logging.h>
#include <unordered_map>
#include <type_traits>
#include <string>
#include <typeinfo>

namespace Serialize {

  template<typename T>
  void WriteClass(const T &value, FILE *file) {
    value.SaveOrDie(file);
  };

  template<class T>
  void Write(const T &value, FILE *file) {
    if (std::is_integral<T>::value || std::is_floating_point<T>::value) {
      CHECK_EQ(1, fwrite(&value, sizeof(T), 1, file));
    } else if (std::is_enum<T>::value) {
      CHECK_EQ(1, fwrite(&value, sizeof(int), 1, file));
    } else {
      LOG(FATAL) << "Unsupported type: " << typeid(T).name();
    }
  }

  template<class V, class K>
  void Write(const std::pair<V, K>& value, FILE* file) {
    Write(value.first, file);
    Write(value.second, file);
  }

  template <class T>
  void ReadClass(T* value, FILE* file) {
    value->LoadOrDie(file);
  }

  template <class T>
  void Read(T* value, FILE* file) {
    if (std::is_integral<T>::value || std::is_floating_point<T>::value) {
      CHECK_EQ(1, fread(value, sizeof(T), 1, file));
    } else if (std::is_enum<T>::value) {
      CHECK_EQ(1, fread(value, sizeof(int), 1, file));
    } else {
      LOG(FATAL) << "Unsupported type: " << typeid(T).name();
    }
  }

  template<class V, class K>
  void Read(std::pair<V, K>* value, FILE* file) {
    Read(&value->first, file);
    Read(&value->second, file);
  }

  template <class T>
  T Read(FILE* file) {
    T value;
    Read(&value, file);
    return value;
  }

  template<>
  void Write<std::string>(const std::string& value, FILE* file);

  template<>
  void Read<std::string>(std::string* value, FILE* file);

  template<class V>
  void WriteVector(const std::vector<V>& values, FILE* file) {
    Write(values.size(), file);
    for (const V& entry : values) {
      Write(entry, file);
    }
  }

  template<class V>
  void ReadVector(std::vector<V>* values, FILE* file) {
    values->clear();
    size_t size = Read<size_t>(file);
    values->resize(size);
    for (size_t i = 0; i < size; i++) {
      Read(&(*values)[i], file);
    }
  }

  template<class V>
  void WriteVectorClass(const std::vector<V>& values, FILE* file) {
    Write(values.size(), file);
    for (const V& entry : values) {
      WriteClass(entry, file);
    }
  }

  template<class V>
  void ReadVectorClass(std::vector<V>* values, FILE* file) {
    values->clear();
    size_t size = Read<size_t>(file);
    for (size_t i = 0; i < size; i++) {
      V entry;
      ReadClass(&entry, file);
      values->emplace_back(std::move(entry));
    }
  }

  template<template <typename...> class Map, typename K, typename V>
  void WriteMap(const Map<K, V>& map, FILE* file) {
    Write(map.size(), file);
    for (const auto& it : map) {
      Write(it.first, file);
      Write(it.second, file);
    }
  }

  template<template <typename...> class Map, typename K, typename V>
  void ReadMap(Map<K, V>* map, FILE* file) {
    map->clear();
    size_t size = Read<size_t>(file);

    K key;
    V value;
    for (size_t i = 0; i < size; i++) {
      Read(&key, file);
      Read(&value, file);
      map->emplace(key, value);
    }
  }


}




#endif //CC_SYNTHESIS_SERIALIZEUTIL_H
