#include <cstddef>
#include <iostream>
#include <vector>

struct Point {
  double x, y, z;
  bool operator==(const Point &p) const {
    return x == p.x && y == p.y && z == p.z;
  }
};

struct BoundingBox {
  Point center;
  double halfSize;

  bool contains(const Point &point) const {
    return (point.x >= center.x - halfSize && point.x <= center.x + halfSize &&
            point.y >= center.y - halfSize && point.y <= center.y + halfSize &&
            point.z >= center.z - halfSize && point.z <= center.z + halfSize);
  }
};

class OctreeNode {
 private:
  BoundingBox boundary;
  std::vector<Point> points;
  OctreeNode *children[8];
  unsigned int capacity;
  bool isLeaf;

 public:
  OctreeNode(const BoundingBox &_boundary, int _capacity)
      : boundary(_boundary), capacity(_capacity), isLeaf(true) {
    for (size_t i = 0; i < 8; i++) {
      children[i] = nullptr;
    }
  }

  ~OctreeNode() {
    for (size_t i = 0; i < 8; i++) {
      delete children[i];
    }
  }

  bool insert(const Point &point) {
    if (!boundary.contains(point)) return false;

    if (isLeaf) {
      if (points.size() < capacity) {
        points.push_back(point);
        return true;
      }

      subdivide();
    }

    for (size_t i = 0; i < 8; i++) {
      if (children[i]->insert(point)) return true;
    }

    return false;
  }

  void subdivide() {
    double newHS = boundary.halfSize / 2.0f;
    Point c = boundary.center;

    children[0] = new OctreeNode(
        {{c.x - newHS, c.y - newHS, c.z - newHS}, newHS}, capacity);

    children[1] = new OctreeNode(
        {{c.x + newHS, c.y - newHS, c.z - newHS}, newHS}, capacity);

    children[2] = new OctreeNode(
        {{c.x - newHS, c.y + newHS, c.z - newHS}, newHS}, capacity);

    children[3] = new OctreeNode(
        {{c.x + newHS, c.y + newHS, c.z - newHS}, newHS}, capacity);

    children[4] = new OctreeNode(
        {{c.x - newHS, c.y - newHS, c.z + newHS}, newHS}, capacity);

    children[5] = new OctreeNode(
        {{c.x + newHS, c.y - newHS, c.z + newHS}, newHS}, capacity);

    children[6] = new OctreeNode(
        {{c.x - newHS, c.y + newHS, c.z + newHS}, newHS}, capacity);

    children[7] = new OctreeNode(
        {{c.x + newHS, c.y + newHS, c.z + newHS}, newHS}, capacity);

    /*Actualizamos los puntos si hay nodos hijos*/
    for (const auto &point : points) {
      for (size_t i = 0; i < 8; i++) {
        children[i]->insert(point);
      }
    }

    points.clear();
    isLeaf = false;
  }

  bool search(const Point &point) {
    if (!boundary.contains(point)) return false;

    if (isLeaf) {
      for (const auto &p : points) {
        if (p == point) {
          return true;
        }
      }
    }

    for (size_t i = 0; i < 8; i++) {
      if (children[i] != nullptr) {
        if (children[i]->search(point)) return true;
      }
    }

    return false;
  }
};

int main(int argc, char *argv[]) {
  std::cout << "Hello chupapi\n";

  BoundingBox boundaryRoot = {{0, 0, 0}, 10};
  OctreeNode octree(boundaryRoot, 1);

  octree.insert({1, 2, 3});
  std::cout << (octree.search({1, 2, 2}) ? "Found" : "Not Found") << std::endl;
  return 0;
}
