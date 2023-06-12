#pragma once

#include "CoreMinimal.h"

class OctreeNode {
public:
    FVector origin;
    FVector halfDimension;
    FVector starPosition;
    double starRadius; // Новая переменная для хранения радиуса звезды
    OctreeNode* children[8];

    OctreeNode(const FVector& origin, const FVector& halfDimension)
        : origin(origin), halfDimension(halfDimension), starPosition(0, 0, 0), starRadius(0)
    {
        for (int i = 0; i < 8; ++i) children[i] = nullptr;
    }

    ~OctreeNode() 
    {
        for (int i = 0; i < 8; ++i)
            delete children[i];
    }

    bool isLeafNode() const 
    {
        for (int i = 0; i < 8; ++i)
            if (children[i] != nullptr)
                return false;
        return true;
    }
};

class Octree 
{
public:
    OctreeNode* root;

    Octree(const FVector& origin, const FVector& halfDimension)
        : root(new OctreeNode(origin, halfDimension)) {}

    ~Octree() 
    {
        delete root;
    }

    // Вставить звезду в октодерево. Эта функция возвращает true, если вставка прошла успешно, и false в противном случае.
    bool InsertStar(const FVector& position, float radius) 
    {
        // Если корневой узел достаточно большой, чтобы вместить звезду, мы вставляем ее туда.
        if (root->halfDimension.X >= radius) {
            InsertStarIntoNode(root, position, radius);
            return true;
        }
        // Если корневой узел слишком мал, чтобы вместить звезду, вставка не удается.
        else {
            return false;
        }
    }

    // Вспомогательная функция для вставки звезды в узел. Эта функция разделяет узел на восемь подузлов, если это необходимо.
    void InsertStarIntoNode(OctreeNode* node, const FVector& position, float radius) 
    {
        // Если узел является листовым и пустым, мы просто вставляем звезду в него.
        if (node->isLeafNode() && !SpaceOccupiedInNode(node, position, radius)) { // добавляем проверку на занятость
            node->starPosition = position;
            node->starRadius = radius; // Сохраняем радиус звезды
        }
        // Если узел уже содержит звезду, или он слишком мал, чтобы вместить новую звезду, мы разделяем его на восемь подузлов.
        else {
            // Если узел является листовым, но уже содержит звезду, мы вставляем эту звезду в один из новых подузлов.
            if (node->isLeafNode()) 
            {
                FVector oldStarPosition = node->starPosition;
                node->starPosition = FVector(0, 0, 0);
                for (int i = 0; i < 8; ++i) 
                {
                    // Для нового оригина и половины размера, мы берем текущие значения узла и меняем их в зависимости от индекса подузла.
                    // Это будет создавать 8 подузлов в разных углах узла.
                    FVector newOrigin = node->origin + FVector((i & 1) ? node->halfDimension.X : -node->halfDimension.X,
                        (i & 2) ? node->halfDimension.Y : -node->halfDimension.Y,
                        (i & 4) ? node->halfDimension.Z : -node->halfDimension.Z) * 0.5f;
                    FVector newHalfDimension = node->halfDimension * 0.5f;

                    node->children[i] = new OctreeNode(newOrigin, newHalfDimension);

                    // Если подузел содержит старую позицию звезды, мы присваиваем ее этому подузлу.
                    // Мы просто проверяем, находится ли старая позиция звезды внутри этого подузла.
                    if (Contains(node->children[i], oldStarPosition)) 
                    {
                        node->children[i]->starPosition = oldStarPosition;
                        break;
                    }
                }
            }
            // Пытаемся вставить новую звезду в каждый из подузлов.
            for (int i = 0; i < 8; ++i) {
                // Если подузел содержит новую позицию звезды, мы пытаемся вставить ее в этот подузел.
                if (node && node->children[i] && Contains(node->children[i], position)) 
                {
                    InsertStarIntoNode(node->children[i], position, radius);
                    break;
                }
            }
        }
    }

    bool Contains(OctreeNode* node, const FVector& position) 
    {
        // Проверяем, находится ли позиция внутри куба, представленного узлом.
        if (position.X < node->origin.X - node->halfDimension.X || position.X > node->origin.X + node->halfDimension.X)
            return false;
        if (position.Y < node->origin.Y - node->halfDimension.Y || position.Y > node->origin.Y + node->halfDimension.Y)
            return false;
        if (position.Z < node->origin.Z - node->halfDimension.Z || position.Z > node->origin.Z + node->halfDimension.Z)
            return false;

        return true;
    }

    bool SpaceOccupied(const FVector& position, float radius) 
    {
        return SpaceOccupiedInNode(root, position, radius);
    }
    bool SpaceOccupiedInNode(OctreeNode* node, const FVector& position, float radius) {
        // Если узел не содержит позиции звезды и радиуса звезды, пространство свободно.
        if (node->starPosition == FVector(0, 0, 0) && node->starRadius == 0)
            return false;

        // Если узел содержит позицию звезды, но радиус этой звезды меньше радиуса проверяемого пространства, пространство свободно.
        float distanceBetweenStars = FVector::Dist(node->starPosition, position);
        if (distanceBetweenStars > (node->starRadius + radius)) // проверка на пересечение
            return false;

        // Если узел содержит позицию звезды, и радиус этой звезды больше или равен радиусу проверяемого пространства, пространство занято.
        return true;
    }
    //bool SpaceOccupiedInNode(OctreeNode* node, const FVector& position, float radius) {
    //    // Если узел не содержит позиции звезды, пространство свободно.
    //    if (node->starPosition == FVector(0, 0, 0))
    //        return false;

    //    // Если узел содержит позицию звезды, но радиус этой звезды меньше радиуса проверяемого пространства, пространство свободно.
    //    float distanceBetweenStars = FVector::Dist(node->starPosition, position);
    //    if (distanceBetweenStars > radius)
    //        return false;

    //    // Если узел содержит позицию звезды, и радиус этой звезды больше или равен радиусу проверяемого пространства, пространство занято.
    //    return true;
    //}
};
