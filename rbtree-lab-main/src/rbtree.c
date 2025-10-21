#include "rbtree.h"

#include <stdlib.h>

rbtree *new_rbtree(void) {
    rbtree *p = (rbtree *)calloc(1, sizeof(rbtree));
    // TODO: initialize struct if needed
    node_t* nil=(node_t *)calloc(1, sizeof(node_t));
    nil->color=RBTREE_BLACK;
	nil->left = nil->right = nil->parent = nil;
    nil->key=0;

    p->root=nil;
    p->nil=nil;
    return p;
}

void delete_rbtree(rbtree *t) {
    // TODO: reclaim the tree nodes's memory  
    // delete_tree(tree): RB tree 구조체가 차지했던 메모리 반환
    // 해당 tree가 사용했던 메모리를 전부 반환해야 합니다. (valgrind로 나타나지 않아야 함)

    //루트에서 밑으로 순회하면서 하나씩 free
	if (t == NULL) return;

	delete_dfs(t,t->root);
	free(t->nil);
    free(t);
}
void delete_dfs(rbtree* t, node_t* node){
	if(node==t->nil) return;
	delete_dfs(t,node->left);
	delete_dfs(t,node->right);
	
	free(node);
}

node_t *rbtree_insert(rbtree *t, const key_t key) {
    // TODO: implement insert
    // tree_insert(tree, key): key 추가
    // 구현하는 ADT가 multiset이므로 이미 같은 key의 값이 존재해도 하나 더 추가 합니다.
	node_t *newNode=(node_t*)calloc(1,sizeof(node_t));
	newNode->color = RBTREE_RED;
	newNode->key = key;
	newNode->parent = newNode->left = newNode->right = t->nil;

    node_t *res = t->nil;
    node_t *temp = t->root;
    while (temp != t->nil) {
        res = temp;
        if (newNode->key < temp->key){
            temp = temp->left;
		}else{
            temp = temp->right;
		}
    }
	newNode->parent=res;
	if(res==t->nil){t->root=newNode;}
	else if(key<res->key){res->left=newNode;}
	else{res->right=newNode;}

	insert_fixup(t,newNode);
	t->root->color=RBTREE_BLACK;//루트 블랙으로
	return newNode;
}
void insert_fixup(rbtree *t, node_t *tempNode) {
    while (tempNode->parent != t->nil && tempNode->parent->color == RBTREE_RED) {
        node_t *parent = tempNode->parent;
        node_t *grandparent = parent->parent;

        if (grandparent == t->nil) break;  // 안전하게 루트 위로 넘어가지 않도록

        // 부모가 할아버지의 왼쪽 자식인 경우
        if (parent == grandparent->left) {
            node_t *uncle = grandparent->right;

            // Case 1: 삼촌이 RED
            if (uncle->color == RBTREE_RED) {
                parent->color = RBTREE_BLACK;
                uncle->color = RBTREE_BLACK;
                grandparent->color = RBTREE_RED;
                tempNode = grandparent;
                continue;
            }

            // Case 2: 삼촌이 BLACK이고 tempNode가 부모의 오른쪽 자식
            if (tempNode == parent->right) {
                tempNode = parent;
                left_rotate(t, tempNode);
                parent = tempNode->parent; // 회전 후 parent 업데이트
            }

            // Case 3: 삼촌이 BLACK이고 tempNode가 부모의 왼쪽 자식
            parent->color = RBTREE_BLACK;
            grandparent->color = RBTREE_RED;
            right_rotate(t, grandparent);
        }
        // 부모가 할아버지의 오른쪽 자식인 경우 (대칭)
        else {
            node_t *uncle = grandparent->left;

            // Case 1: 삼촌이 RED
            if (uncle->color == RBTREE_RED) {
                parent->color = RBTREE_BLACK;
                uncle->color = RBTREE_BLACK;
                grandparent->color = RBTREE_RED;
                tempNode = grandparent;
                continue;
            }

            // Case 2: 삼촌이 BLACK이고 tempNode가 부모의 왼쪽 자식
            if (tempNode == parent->left) {
                tempNode = parent;
                right_rotate(t, tempNode);
                parent = tempNode->parent; // 회전 후 parent 업데이트
            }

            // Case 3: 삼촌이 BLACK이고 tempNode가 부모의 오른쪽 자식
            parent->color = RBTREE_BLACK;
            grandparent->color = RBTREE_RED;
            left_rotate(t, grandparent);
        }
    }

    // 루트는 항상 BLACK
    t->root->color = RBTREE_BLACK;
}

void left_rotate(rbtree* t,node_t *node){
	node_t *right = node->right;
	if(!t||!node||right==t->nil) return;
	if(node==t->root){
		t->root=right;
	} else {
		if(node->parent->left==node){
			node->parent->left=right;
		} else {
			node->parent->right=right;
		}
	}
	right->parent=node->parent;

	if(right->left!=t->nil){
		right->left->parent=node;
	}
	node->right=right->left;

	node->parent=right;
	right->left=node;
}
void right_rotate(rbtree* t,node_t *node){
	node_t *left = node->left;
	if(!t||!node||left==t->nil) return;
	if(node==t->root){
		t->root=left;
	} else {
		if(node->parent->right==node){
			node->parent->right=left;
		} else {
			node->parent->left=left;
		}
	}
	left->parent=node->parent;

	if(left->right!=t->nil){
		left->right->parent=node;
	}
	node->left=left->right;

	node->parent=left;
	left->right=node;
}


node_t *rbtree_find(const rbtree *t, const key_t key) {
    // TODO: implement find
    // ptr = tree_find(tree, key)
    // RB tree내에 해당 key가 있는지 탐색하여 있으면 해당 node pointer 반환
    // 해당하는 node가 없으면 NULL 반환
	node_t *temp = t->root;
	if (temp == t->nil) return NULL;
  	while (temp != t->nil) {
		if (key == temp->key) {
			return temp;
		} else if (key < temp->key) {
			temp = temp->left;
		} else {
			temp = temp->right;
		}
  	}
  	return NULL;
}

node_t *rbtree_min(const rbtree *t) {
    // TODO: implement find
    // ptr = tree_min(tree): RB tree 중 최소 값을 가진 node pointer 반환
	node_t *temp=t->root;
	while(temp->left!=t->nil){
		temp=temp->left;
	}
    return temp;
}

node_t *rbtree_max(const rbtree *t) {
    // TODO: implement find
    // ptr = tree_max(tree): 최대값을 가진 node pointer 반환
    node_t *temp=t->root;
	while(temp->right!=t->nil){
		temp=temp->right;
	}
    return temp;
}

int rbtree_erase(rbtree *t, node_t *p) {
    // TODO: implement erase
    // tree_erase(tree, ptr): RB tree 내부의 ptr로 지정된 node를 삭제하고 메모리 반환
	//오른쪽의 가장 왼쪽을 올림
	if (!t || !p || p == t->nil) return -1;

	node_t *replaceNode;//p대신 들어갈 노드
	node_t *replaceChild; //replaceNode대신 들어갈 노드
    color_t originalColor = p->color;

	if (p->left == t->nil) {  // case 1: 왼쪽 자식 없음
        replaceChild = p->right;
        transplant(t, p, p->right);
    } 
    else if (p->right == t->nil) {  // case 2: 오른쪽 자식 없음
        replaceChild = p->left;
        transplant(t, p, p->left);
    } 
    else {  // case 3: 양쪽 자식 모두 있음
        replaceNode = p->right;
        while (replaceNode->left != t->nil)
            replaceNode = replaceNode->left;

        originalColor = replaceNode->color;
        replaceChild = replaceNode->right;

        if (replaceNode->parent == p) {
            replaceChild->parent = replaceNode;
        } else {
            transplant(t, replaceNode, replaceChild);
            replaceNode->right = p->right;
            replaceNode->right->parent = replaceNode;
			replaceNode->left->parent = replaceNode;
			replaceChild->parent = replaceNode->parent;
        }

        transplant(t, p, replaceNode);
        replaceNode->left = p->left;
        replaceNode->left->parent = replaceNode;
        replaceNode->color = p->color;
    }

    if (originalColor == RBTREE_BLACK)
        erase_fixup(t, replaceChild);

    free(p);
    return 0;
}
// u를 v로 교체 (v는 u의 자리를 대체)
void transplant(rbtree *t, node_t *u, node_t *v) {
    if (u->parent == t->nil)
        t->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    v->parent = u->parent;
}
void erase_fixup(rbtree *t, node_t *node) {
    node_t *bro;

    while (node != t->root && node->color == RBTREE_BLACK) {
        if (node == node->parent->left) {
            bro = node->parent->right;

            // Case 1: 형제가 RED
            if (bro->color == RBTREE_RED) {
                bro->color = RBTREE_BLACK;
                node->parent->color = RBTREE_RED;
                left_rotate(t, node->parent);
                bro = node->parent->right;
            }

            // Case 2: 형제가 BLACK이고, 두 자식도 BLACK
            if (bro->left->color == RBTREE_BLACK && bro->right->color == RBTREE_BLACK) {
                bro->color = RBTREE_RED;
                node = node->parent;
            } else {
                // Case 3: 형제가 BLACK이고, 왼쪽 자식은 RED, 오른쪽은 BLACK
                if (bro->right->color == RBTREE_BLACK) {
                    bro->left->color = RBTREE_BLACK;
                    bro->color = RBTREE_RED;
                    right_rotate(t, bro);
                    bro = node->parent->right;
                }
                // Case 4: 형제가 BLACK이고, 오른쪽 자식이 RED
                bro->color = node->parent->color;
                node->parent->color = RBTREE_BLACK;
                bro->right->color = RBTREE_BLACK;
                left_rotate(t, node->parent);
                node = t->root;
            }
        } else { // node == node->parent->right (대칭)
            bro = node->parent->left;

            // Case 1: 형제가 RED
            if (bro->color == RBTREE_RED) {
                bro->color = RBTREE_BLACK;
                node->parent->color = RBTREE_RED;
                right_rotate(t, node->parent);
                bro = node->parent->left;
            }

            // Case 2: 형제가 BLACK이고, 두 자식도 BLACK
            if (bro->right->color == RBTREE_BLACK && bro->left->color == RBTREE_BLACK) {
                bro->color = RBTREE_RED;
                node = node->parent;
            } else {
                // Case 3: 형제가 BLACK이고, 왼쪽 자식은 RED, 오른쪽은 BLACK
                if (bro->left->color == RBTREE_BLACK) {
                    bro->right->color = RBTREE_BLACK;
                    bro->color = RBTREE_RED;
                    left_rotate(t, bro);
                    bro = node->parent->left;
                }
                // Case 4: 형제가 BLACK이고, 오른쪽 자식이 RED
                bro->color = node->parent->color;
                node->parent->color = RBTREE_BLACK;
                bro->left->color = RBTREE_BLACK;
                right_rotate(t, node->parent);
                node = t->root;
            }
        }
    }
    node->color = RBTREE_BLACK; // <- 이미 해놨지만, node가 nil일 때 t->root도 확실히 BLACK 처리 필요
	t->root->color = RBTREE_BLACK; // <- 추가로 강제 보장
}

int rbtree_to_array(const rbtree *t, key_t *arr, const size_t n) {
	// TODO: implement to_array
    // tree_to_array(tree, array, n)
    // RB tree의 내용을 key 순서대로 주어진 array로 변환
    // array의 크기는 n으로 주어지며 tree의 크기가 n 보다 큰 경우에는 순서대로 n개 까지만 변환
    // array의 메모리 공간은 이 함수를 부르는 쪽에서 준비하고 그 크기를 n으로 알려줍니다.
	if (t == NULL) return -1;
	
	int idx = 0;
  	inorder_fill(t->root, t->nil, arr, &idx, n);

  	return idx;
}
void inorder_fill(node_t *node, node_t *nil, key_t *arr, int *idx, const size_t n) {
	  if (node == nil || *idx >= n) return;

	  inorder_fill(node->left, nil, arr, idx, n);
	  arr[(*idx)++] = node->key;
	  inorder_fill(node->right, nil, arr, idx, n);
}
