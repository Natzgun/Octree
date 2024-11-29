#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <queue>
#include <vector>

#include "stb_image.h"
#include "stb_image_write.h"

struct Color {
  uint8_t r, g, b;

  bool operator==(const Color &other) const {
    return r == other.r && g == other.g && b == other.b;
  }

  bool operator<(const Color &other) const { return r < other.r; }
};

struct OctreeColorNode {
  unsigned int pixelCount = 0;
  unsigned long redSum = 0, greenSum = 0, blueSum = 0;
  OctreeColorNode *children[8] = {nullptr};
  bool isLeaf = false;

  ~OctreeColorNode() {
    for (auto &child : children) {
      delete child;
    }
  }

  void addColor(const Color &color, int depth = 0) {
    if (depth == 8) {
      redSum += color.r;
      greenSum += color.g;
      blueSum += color.b;
      pixelCount++;
      isLeaf = true;
      return;
    }

    int index = ((color.r >> (7 - depth)) & 1) << 2 |
                ((color.g >> (7 - depth)) & 1) << 1 |
                ((color.b >> (7 - depth)) & 1);

    if (children[index] == nullptr) {
      children[index] = new OctreeColorNode();
    }
    children[index]->addColor(color, depth + 1);
  }

  void reduce(
      std::priority_queue<std::pair<int, OctreeColorNode *>,
                          std::vector<std::pair<int, OctreeColorNode *> >,
                          std::greater<> > &leafNodes) {
    for (auto &child : children) {
      if (child != nullptr) {
        if (child->isLeaf) {
          leafNodes.emplace(child->pixelCount, child);
        } else {
          child->reduce(leafNodes);
        }
      }
    }
  }
};

class OctreeColorQuantizer {
 private:
  OctreeColorNode *root;
  int maxColors;

 public:
  OctreeColorQuantizer(int _maxColors) : maxColors(_maxColors) {
    root = new OctreeColorNode();
  }

  ~OctreeColorQuantizer() { delete root; }

  void addColor(const Color &color) { root->addColor(color); }

  std::vector<Color> getPalette() {
    std::priority_queue<std::pair<int, OctreeColorNode *>,
                        std::vector<std::pair<int, OctreeColorNode *> >,
                        std::greater<> >
        leafNodes;
    root->reduce(leafNodes);

    while (leafNodes.size() > maxColors) {
      auto [_, node] = leafNodes.top();
      leafNodes.pop();
      for (auto &child : node->children) {
        if (child != nullptr) {
          leafNodes.emplace(child->pixelCount, child);
        }
      }
    }

    std::vector<Color> palette;
    while (!leafNodes.empty()) {
      auto [_, node] = leafNodes.top();
      leafNodes.pop();
      Color avgColor = {static_cast<uint8_t>(node->redSum / node->pixelCount),
                        static_cast<uint8_t>(node->greenSum / node->pixelCount),
                        static_cast<uint8_t>(node->blueSum / node->pixelCount)};
      palette.push_back(avgColor);
    }
    return palette;
  }
  void imgPaleta() {
    std::vector<Color> palette = getPalette();
    std::sort(palette.begin(), palette.end());
    int numColors = palette.size();
    int n = static_cast<int>(std::ceil(std::sqrt(numColors)));

    std::vector<unsigned char> image(n * n * 3, 255);  // Initialize with white

    for (int i = 0; i < numColors; ++i) {
      int x = i % n;
      int y = i / n;
      int index = (y * n + x) * 3;
      image[index] = palette[i].r;
      image[index + 1] = palette[i].g;
      image[index + 2] = palette[i].b;
    }

    stbi_write_png("./result/output_square_image.png", n, n, 3, image.data(), n * 3);
  }
};

int main() {
  int width, height, channels;
  unsigned char *img =
      stbi_load("./images/bugy.png", &width,
                &height, &channels, 3);
  if (img == nullptr) {
    std::cerr << "Error al cargar la imagen" << std::endl;
    return 1;
  }

  OctreeColorQuantizer quantizer(3);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int index = (y * width + x) * 3;
      Color color = {img[index], img[index + 1], img[index + 2]};
      quantizer.addColor(color);
    }
  }

  std::vector<Color> palette = quantizer.getPalette();

  std::vector<unsigned char> newImage(width * height * 3);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int index = (y * width + x) * 3;
      Color color = {img[index], img[index + 1], img[index + 2]};
      Color closestColor = palette[0];
      int minDistance = INT_MAX;
      for (const auto &paletteColor : palette) {
        int distance = (color.r - paletteColor.r) * (color.r - paletteColor.r) +
                       (color.g - paletteColor.g) * (color.g - paletteColor.g) +
                       (color.b - paletteColor.b) * (color.b - paletteColor.b);
        if (distance < minDistance) {
          minDistance = distance;
          closestColor = paletteColor;
        }
      }
      newImage[index] = closestColor.r;
      newImage[index + 1] = closestColor.g;
      newImage[index + 2] = closestColor.b;
    }
  }

  stbi_write_png("./result/output_image.png", width, height, 3, newImage.data(),
                 width * 3);
  quantizer.imgPaleta();
  stbi_image_free(img);

  return 0;
}
