#include "topology.h"

/* 
 * Node 클래스 맴버함수
 */


/* 
 * 노드 생성자
 * ip주소를 받음
 */
Node::Node(std::string ip)
{
	this->ip = ip;
}

/*
 * ip를 반환하는 함수
 */

std::string Node::getIp()
{
	return ip;
}


/*
 * Tree 클래스 맴버함수
 */

/*
 * 트리 생성자
 * 파일로부터 트리 정보를 받아와야 함
 */

Tree::Tree()
{

}

/*
 * 부모 노드를 반환하는 함수
 * 부모 노드는 하나라고 가정하기 때문에 Node 클래스 리턴
 */

Node Tree::getParent()
{
	return parent;
}

/*
 * 자식 노드를 반환하는 함수
 * 자식은 여러개 있을 수 있기 때문에 벡터로 리턴
 */

std::vector<Node> Tree::child()
{
	return child;
}

/*
 * 피어노드를 반환하는 함수
 * 피어노드는 여러개 있을 수 있어 벡터로 리턴
 */

std::vector<Node> Tree::peer()
{
	return peer;
}