#include "data/data.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <memory>
#include <random>
#include <tuple>
#include <vector>

#include "utils/exception.h"

namespace KD {
namespace SourceData {
unsigned int __reverse_int(int i) {
  unsigned char ch1, ch2, ch3, ch4;
  ch1 = i & 255;
  ch2 = (i >> 8) & 255;
  ch3 = (i >> 16) & 255;
  ch4 = (i >> 24) & 255;
  return ((int)ch1 << 24) + ((int)ch2 << 16) + ((int)ch3 << 8) + ch4;
}

MNIST::MNIST(const std::string &img_path, const std::string &label_path,
             Index batch_size, bool shuffle)
    : batch_size_(batch_size) {
  ReadMnistImages(img_path);
  ReadMnistLabels(label_path);
  batches_size_ = (samples_size_.size() + batch_size - 1) / batch_size;

  if (shuffle) {
    this->Shuffle();
  }
}

std::pair<const BasicData *, Index> MNIST::GetSample(Index idx) const {
  return {reinterpret_cast<const BasicData *>(&samples_size_[idx]), labels_[idx]};
}

std::tuple<Index, const BasicData *, const Index *> MNIST::GetBatch(
    Index idx) const {
  Index n_samples =
      (idx == batches_size_ - 1) ? samples_size_.size() - idx * batch_size_ : batch_size_;
  return {n_samples,
          reinterpret_cast<const BasicData *>(&samples_size_[idx * batch_size_]),
          &labels_[idx * batch_size_]};
}

void MNIST::Shuffle() {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

  std::default_random_engine engine1(seed);
  std::shuffle(samples_size_.begin(), samples_size_.end(), engine1);

  std::default_random_engine engine2(seed);
  std::shuffle(labels_.begin(), labels_.end(), engine2);
}

void MNIST::ReadMnistImages(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    THROW_ERROR("Can't open file: " << path);
  }

  unsigned int headers[4];
  file.read(reinterpret_cast<char *>(headers), 16);
  Index magic_number = __reverse_int(headers[0]);
  Index n_imgs = __reverse_int(headers[1]);
  Index n_bytes = n_imgs * Img::n_pixels_;

  auto char_data_ptr = std::unique_ptr<char[]>(new char[n_bytes]);
  file.read(char_data_ptr.get(), n_bytes);
  auto uchar_data = reinterpret_cast<unsigned char *>(char_data_ptr.get());

  samples_size_.reserve(n_imgs);
  for (Index i = 0; i < n_imgs; ++i) {
    samples_size_.push_back({});
    auto dist =
        reinterpret_cast<BasicData *>(samples_size_.data()) + i * Img::n_pixels_;
    auto src = uchar_data + i * Img::n_pixels_;
    for (Index j = 0; j < Img::n_pixels_; ++j) dist[j] = src[j] / 255.0;
  }
}

void MNIST::ReadMnistLabels(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    THROW_ERROR("Can't open file: " << path);
  }

  unsigned int headers[2];
  file.read(reinterpret_cast<char *>(headers), 8);
  Index magic_number = __reverse_int(headers[0]);
  Index n_imgs = __reverse_int(headers[1]);
  Index n_bytes = n_imgs;

  auto char_data_ptr = std::unique_ptr<char[]>(new char[n_bytes]);
  file.read(char_data_ptr.get(), n_bytes);
  auto uchar_data = reinterpret_cast<unsigned char *>(char_data_ptr.get());

  labels_.reserve(n_imgs);
  for (Index i = 0; i < n_imgs; ++i) {
    Index label = uchar_data[i];
    labels_.push_back(label);
  }
}
}  // namespace SourceData
}  // namespace KD

namespace KD {
namespace SourceData {
Cifar10::Cifar10(const std::string &dataset_dir, bool train, Index batch_size,
                 bool shuffle, char path_sep)
    : batch_size_(batch_size) {
  ReadCifar10(dataset_dir, train, path_sep);
  n_batchs_ = (imgs_.size() + batch_size_ - 1) / batch_size_;
}

std::pair<const BasicData *, Index> Cifar10::GetSample(Index idx) const {
  return {reinterpret_cast<const BasicData *>(&imgs_[idx]), labels_[idx]};
}

std::tuple<Index, const BasicData *, const Index *> Cifar10::GetBatch(
    Index idx) const {
  Index n_samples =
      (idx == n_batchs_ - 1) ? imgs_.size() - idx * batch_size_ : batch_size_;
  return {n_samples,
          reinterpret_cast<const BasicData *>(&imgs_[idx * batch_size_]),
          &labels_[idx * batch_size_]};
}

void Cifar10::Shuffle() {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

  std::default_random_engine engine1(seed);
  std::shuffle(imgs_.begin(), imgs_.end(), engine1);

  std::default_random_engine engine2(seed);
  std::shuffle(labels_.begin(), labels_.end(), engine2);
}

void Cifar10::ReadCifar10(const std::string &dataset_dir, bool train,
                          char path_sep) {
  if (train) {
    imgs_.reserve(Img::n_train_samples_);
    labels_.reserve(Img::n_train_samples_);
    std::vector<std::string> bin_names{"data_batch_1.bin", "data_batch_2.bin",
                                       "data_batch_3.bin", "data_batch_4.bin",
                                       "data_batch_5.bin"};
    for (auto &bin_name : bin_names) ReadBin(dataset_dir + path_sep + bin_name);
  } else {
    imgs_.reserve(Img::n_test_samples_);
    labels_.reserve(Img::n_test_samples_);
    const std::string bin_name = "test_batch.bin";
    ReadBin(dataset_dir + path_sep + bin_name);
  }
}

void Cifar10::ReadBin(const std::string &bin_path) {
  std::ifstream file(bin_path, std::ios::binary);
  if (!file.is_open()) {
    THROW_ERROR("Can't open file: " << bin_path);
  }

  Index sample_size = 1 + Img::n_pixels_;
  std::unique_ptr<char> char_data_ptr(new char[sample_size]);
  auto char_data = char_data_ptr.get();
  auto uchar_data = reinterpret_cast<unsigned char *>(char_data);

  while (file.peek() != EOF) {
    file.read(char_data, sample_size);

    imgs_.push_back({});
    labels_.push_back(uchar_data[0]);

    auto src = uchar_data + 1;
    auto dist = reinterpret_cast<BasicData *>(&imgs_.back());

    for (Index i = 0; i < Img::n_pixels_; ++i) dist[i] = src[i] / 255.0;
  }
}
}  // namespace SourceData
}  // namespace KD