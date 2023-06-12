#pragma once

#include "CoreMinimal.h"

class OctreeNode {
public:
    FVector origin;
    FVector halfDimension;
    FVector starPosition;
    double starRadius; // ����� ���������� ��� �������� ������� ������
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

    // �������� ������ � ����������. ��� ������� ���������� true, ���� ������� ������ �������, � false � ��������� ������.
    bool InsertStar(const FVector& position, float radius) 
    {
        // ���� �������� ���� ���������� �������, ����� �������� ������, �� ��������� �� ����.
        if (root->halfDimension.X >= radius) {
            InsertStarIntoNode(root, position, radius);
            return true;
        }
        // ���� �������� ���� ������� ���, ����� �������� ������, ������� �� �������.
        else {
            return false;
        }
    }

    // ��������������� ������� ��� ������� ������ � ����. ��� ������� ��������� ���� �� ������ ��������, ���� ��� ����������.
    void InsertStarIntoNode(OctreeNode* node, const FVector& position, float radius) 
    {
        // ���� ���� �������� �������� � ������, �� ������ ��������� ������ � ����.
        if (node->isLeafNode() && !SpaceOccupiedInNode(node, position, radius)) { // ��������� �������� �� ���������
            node->starPosition = position;
            node->starRadius = radius; // ��������� ������ ������
        }
        // ���� ���� ��� �������� ������, ��� �� ������� ���, ����� �������� ����� ������, �� ��������� ��� �� ������ ��������.
        else {
            // ���� ���� �������� ��������, �� ��� �������� ������, �� ��������� ��� ������ � ���� �� ����� ��������.
            if (node->isLeafNode()) 
            {
                FVector oldStarPosition = node->starPosition;
                node->starPosition = FVector(0, 0, 0);
                for (int i = 0; i < 8; ++i) 
                {
                    // ��� ������ ������� � �������� �������, �� ����� ������� �������� ���� � ������ �� � ����������� �� ������� �������.
                    // ��� ����� ��������� 8 �������� � ������ ����� ����.
                    FVector newOrigin = node->origin + FVector((i & 1) ? node->halfDimension.X : -node->halfDimension.X,
                        (i & 2) ? node->halfDimension.Y : -node->halfDimension.Y,
                        (i & 4) ? node->halfDimension.Z : -node->halfDimension.Z) * 0.5f;
                    FVector newHalfDimension = node->halfDimension * 0.5f;

                    node->children[i] = new OctreeNode(newOrigin, newHalfDimension);

                    // ���� ������� �������� ������ ������� ������, �� ����������� �� ����� �������.
                    // �� ������ ���������, ��������� �� ������ ������� ������ ������ ����� �������.
                    if (Contains(node->children[i], oldStarPosition)) 
                    {
                        node->children[i]->starPosition = oldStarPosition;
                        break;
                    }
                }
            }
            // �������� �������� ����� ������ � ������ �� ��������.
            for (int i = 0; i < 8; ++i) {
                // ���� ������� �������� ����� ������� ������, �� �������� �������� �� � ���� �������.
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
        // ���������, ��������� �� ������� ������ ����, ��������������� �����.
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
        // ���� ���� �� �������� ������� ������ � ������� ������, ������������ ��������.
        if (node->starPosition == FVector(0, 0, 0) && node->starRadius == 0)
            return false;

        // ���� ���� �������� ������� ������, �� ������ ���� ������ ������ ������� ������������ ������������, ������������ ��������.
        float distanceBetweenStars = FVector::Dist(node->starPosition, position);
        if (distanceBetweenStars > (node->starRadius + radius)) // �������� �� �����������
            return false;

        // ���� ���� �������� ������� ������, � ������ ���� ������ ������ ��� ����� ������� ������������ ������������, ������������ ������.
        return true;
    }
    //bool SpaceOccupiedInNode(OctreeNode* node, const FVector& position, float radius) {
    //    // ���� ���� �� �������� ������� ������, ������������ ��������.
    //    if (node->starPosition == FVector(0, 0, 0))
    //        return false;

    //    // ���� ���� �������� ������� ������, �� ������ ���� ������ ������ ������� ������������ ������������, ������������ ��������.
    //    float distanceBetweenStars = FVector::Dist(node->starPosition, position);
    //    if (distanceBetweenStars > radius)
    //        return false;

    //    // ���� ���� �������� ������� ������, � ������ ���� ������ ������ ��� ����� ������� ������������ ������������, ������������ ������.
    //    return true;
    //}
};
