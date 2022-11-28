#include "BehaviourTree.h"

Node::Node(const std::string& name, NodeType type) noexcept
	: m_name{ name },
	m_nodeType{ type },
	m_parent{ nullptr }
{}

void Node::SetParent(Node* pNode) noexcept
{
	m_parent = pNode;
}

Composite::Composite(const std::string& name, NodeType type) noexcept
	: Node{ name, type }
{}

void Composite::AddChild(std::shared_ptr<Node>&& pNode) noexcept
{
	m_children.push_back(pNode);
	m_children.back()->SetParent(this);
}

Sequence::Sequence(const std::string& name) noexcept
	: Composite{ name, NodeType::Sequence }
{}

bool Sequence::Process() noexcept
{
	for (auto& child : GetChildren())
	{
		if (!child->Process())
			return false;
	}
	return true;
}

Selector::Selector(const std::string& name) noexcept
	: Composite{ name, NodeType::Selector }
{}

bool Selector::Process() noexcept
{
	for (auto& child : GetChildren())
	{
		if (child->Process())
			return true;
	}
	return false;
}

Decorator::Decorator(const std::string& name, const DecoratorType type) noexcept
	: Node{ name, NodeType::Decorator },
	m_decoratorType{ type },
	m_child{ nullptr }
{}

void Decorator::AddChild(std::shared_ptr<Node>&& pNode) noexcept
{
	ASSERT(!m_child, "Decorator already has child node assigned.");
	m_child = pNode;
	m_child->SetParent(this);
}

Succeeder::Succeeder(const std::string& name) noexcept
	: Decorator{ name, DecoratorType::Succeeder }
{}

bool Succeeder::Process() noexcept
{
	GetChild()->Process();
	return true;
}

Failer::Failer(const std::string& name) noexcept
	: Decorator{ name, DecoratorType::Failer}
{}

bool Failer::Process() noexcept
{
	GetChild()->Process();
	return false;
}

Inverter::Inverter(const std::string& name) noexcept
	: Decorator{ name, DecoratorType::Inverter}
{}

bool Inverter::Process() noexcept
{
	return (!GetChild()->Process());
}

Root::Root(const std::string& name) noexcept
	: Decorator(name, DecoratorType::Root)
{}

bool Root::Process() noexcept
{
	return GetChild()->Process();
}