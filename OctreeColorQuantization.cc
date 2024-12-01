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
  OctreeColorNode *parent = nullptr;
  OctreeColorNode *children[8] = {nullptr};
  bool isLeaf = false;

  void addColor(const Color &color, int &allColor, int depth = 0) {
    pixelCount++;
    if (depth == 8) {
      redSum += color.r;
      greenSum += color.g;
      blueSum += color.b;
      isLeaf = true;
      if (pixelCount == 1) allColor++;
      return;
    }

    int index = ((color.r >> (7 - depth)) & 1) << 2 |
                ((color.g >> (7 - depth)) & 1) << 1 |
                ((color.b >> (7 - depth)) & 1);

    if (children[index] == nullptr) {
      children[index] = new OctreeColorNode();
      children[index]->parent = this;
    }
    children[index]->addColor(color, allColor, depth + 1);
  }
  // Llamar al metodo solo para verificar que un nodo padre se volvio un nodo
  // hoja;
  bool allChildrenAreLeaf() {
    for (auto &child : children) {
      if (child != nullptr && !child->isLeaf) {
        return false;
      }
    }
    return true;
  }
  void reduce(
      std::priority_queue<std::pair<int, OctreeColorNode *>,
                          std::vector<std::pair<int, OctreeColorNode *> >,
                          std::greater<> > &leafNodes) {
    if (allChildrenAreLeaf()) {
      leafNodes.emplace(this->pixelCount, this);
      return;
    }
    for (auto &child : children) {
      if (child != nullptr) {
        child->reduce(leafNodes);
      }
    }
  }
};

class OctreeColorQuantizer {
 private:
  OctreeColorNode *root;
  int allColor = 0;
  int maxColors;

 public:
  OctreeColorQuantizer(int _maxColors) : maxColors(_maxColors) {
    root = new OctreeColorNode();
  }

  ~OctreeColorQuantizer() { delete root; }

  void addColor(const Color &color) { root->addColor(color, this->allColor); }
  int getAllColor() { return allColor; }

  std::vector<Color> getPalette() {
    std::priority_queue<std::pair<int, OctreeColorNode *>,
                        std::vector<std::pair<int, OctreeColorNode *> >,
                        std::greater<> >
        nodeWithAllChildrenLeaf;
    root->reduce(nodeWithAllChildrenLeaf);
    while (allColor > maxColors) {
      auto [_, node] = nodeWithAllChildrenLeaf.top();
      nodeWithAllChildrenLeaf.pop();

      for (auto &child : node->children) {
        if (child != nullptr) {
          node->redSum += child->redSum;
          node->blueSum += child->blueSum;
          node->greenSum += child->greenSum;
          delete child;
          child = nullptr;
          allColor--;
        }
      }
      node->isLeaf = true;
      allColor++;
      if ((node->parent)->allChildrenAreLeaf()) {
        nodeWithAllChildrenLeaf.emplace(node->parent->pixelCount, node->parent);
      }
    }

    std::vector<Color> palette;

    while (!nodeWithAllChildrenLeaf.empty()) {
      auto [_, node] = nodeWithAllChildrenLeaf.top();
      nodeWithAllChildrenLeaf.pop();
      for (auto &child : node->children) {
        if (child != nullptr) {
          Color avgColor = {
              static_cast<uint8_t>(child->redSum / child->pixelCount),
              static_cast<uint8_t>(child->greenSum / child->pixelCount),
              static_cast<uint8_t>(child->blueSum / child->pixelCount)};
          palette.push_back(avgColor);
        }
      }
    }
    imgPaleta((palette));
    return palette;
  }
  void imgPaleta(std::vector<Color> &palette) {
    std::sort(palette.begin(), palette.end());
    int numColors = palette.size();
    int n = static_cast<int>(std::ceil(std::sqrt(numColors)));

    std::vector<unsigned char> image(n * n * 3, 255);  // Lo inicializamos con blanco

    for (int i = 0; i < numColors; ++i) {
      int x = i % n;
      int y = i / n;
      int index = (y * n + x) * 3;
      image[index] = palette[i].r;
      image[index + 1] = palette[i].g;
      image[index + 2] = palette[i].b;
    }

    stbi_write_png("output_square_image.png", n, n, 3, image.data(), n * 3);
  }
};

int main() {
  int width, height, channels;
  unsigned char *img = stbi_load("../img_6.png", &width, &height, &channels, 3);
  if (img == nullptr) {
    std::cerr << "Error al cargar la imagen" << std::endl;
    return 1;
  }

  OctreeColorQuantizer quantizer(64);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int index = (y * width + x) * 3;
      Color color = {img[index], img[index + 1], img[index + 2]};
      quantizer.addColor(color);
    }
  }
  std::cout << "All colors: " << quantizer.getAllColor() << std::endl;

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

  stbi_write_png("output_image.png", width, height, 3, newImage.data(),
                 width * 3);
  stbi_image_free(img);

  return 0;
}
