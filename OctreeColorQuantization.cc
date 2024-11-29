#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include <cmath>
#include <iostream>
#include <queue>
#include <vector>

struct Color {
  uint8_t r, g, b;

  bool operator==(const Color &other) const {
    return r == other.r && g == other.g && b == other.b;
  }
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
    if (depth == 8) { // Leaf node
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

  void reduce(std::priority_queue<OctreeColorNode *> &leafNodes) {
    for (auto &child : children) {
      if (child != nullptr) {
        if (child->isLeaf) {
          leafNodes.push(child);
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
    std::priority_queue<OctreeColorNode *> leafNodes;
    root->reduce(leafNodes);

    while (leafNodes.size() > maxColors) {
      OctreeColorNode *node = leafNodes.top();
      leafNodes.pop();
      for (auto &child : node->children) {
        if (child != nullptr) {
          leafNodes.push(child);
        }
      }
    }

    std::vector<Color> palette;
    while (!leafNodes.empty()) {
      OctreeColorNode *node = leafNodes.top();
      leafNodes.pop();
      Color avgColor = {
          static_cast<uint8_t>(node->redSum / node->pixelCount),
          static_cast<uint8_t>(node->greenSum / node->pixelCount),
          static_cast<uint8_t>(node->blueSum / node->pixelCount)};
      palette.push_back(avgColor);
    }
    return palette;
  }
};

void savePaletteAsImage(const std::vector<Color> &palette, const char *filename,
                        int tileSize = 50) {
  int width = tileSize * palette.size();
  int height = tileSize;
  std::vector<unsigned char> image(width * height * 3, 0);

  for (size_t i = 0; i < palette.size(); i++) {
    for (int y = 0; y < tileSize; y++) {
      for (int x = 0; x < tileSize; x++) {
        int idx = (y * width + (i * tileSize + x)) * 3;
        image[idx] = palette[i].r;
        image[idx + 1] = palette[i].g;
        image[idx + 2] = palette[i].b;
      }
    }
  }

  stbi_write_png(filename, width, height, 3, image.data(), width * 3);
}

void saveQuantizedImage(const std::vector<Color> &palette, unsigned char *img,
                        int width, int height, const char *filename) {
  std::vector<unsigned char> quantizedImg(width * height * 3);

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int idx = (y * width + x) * 3;
      Color pixel = {img[idx], img[idx + 1], img[idx + 2]};
      Color closest = palette[0];
      double minDist = 1e9;

      for (const auto &color : palette) {
        double dist =
            std::pow(pixel.r - color.r, 2) + std::pow(pixel.g - color.g, 2) +
            std::pow(pixel.b - color.b, 2);
        if (dist < minDist) {
          minDist = dist;
          closest = color;
        }
      }

      quantizedImg[idx] = closest.r;
      quantizedImg[idx + 1] = closest.g;
      quantizedImg[idx + 2] = closest.b;
    }
  }

  stbi_write_png(filename, width, height, 3, quantizedImg.data(), width * 3);
}

int main() {
  int width, height, channels;
  unsigned char *img = stbi_load("bugy.png", &width, &height, &channels, 3);
  if (!img) {
    std::cerr << "Error loading image!" << std::endl;
    return -1;
  }

  OctreeColorQuantizer quantizer(2);

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int idx = (y * width + x) * 3;
      Color color = {img[idx], img[idx + 1], img[idx + 2]};
      quantizer.addColor(color);
    }
  }

  std::vector<Color> palette = quantizer.getPalette();
  savePaletteAsImage(palette, "palette.png");
  saveQuantizedImage(palette, img, width, height, "quantized_image.png");

  std::cout << "Palette and quantized image saved." << std::endl;

  stbi_image_free(img);
  return 0;
}
