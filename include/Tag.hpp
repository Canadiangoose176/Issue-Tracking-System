#ifndef TAG_HPP_
#define TAG_HPP_

#include <string>
#include <utility>
/**
 * @brief Simple value object representing a tag with a name and color.
 */
class Tag {
 private:
  int id_{-1};
  std::string name_;
  std::string color_;

 public:
  Tag() = default;
  explicit Tag(int id) : id_(id) {}
  Tag(std::string name, std::string color)
      : name_(std::move(name)), color_(std::move(color)) {}
  Tag(int id, std::string name, std::string color)
      : id_(id), name_(std::move(name)), color_(std::move(color)) {}

  int getId() const noexcept { return id_; }
  const std::string& getName() const noexcept { return name_; }
  const std::string& getColor() const noexcept { return color_; }

  void setId(int id) { id_ = id; }
  void setName(std::string name) { name_ = std::move(name); }
  void setColor(std::string color) { color_ = std::move(color); }
};

#endif  // TAG_HPP_
