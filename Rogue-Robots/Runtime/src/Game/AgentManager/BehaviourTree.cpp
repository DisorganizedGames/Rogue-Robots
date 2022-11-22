#include "BehaviourTree.h"

Node::Node(const std::string& name) noexcept
	: m_name{ name },
	m_nodeType{ NodeType::Leaf }
{}

Composite::Composite(const std::string& name) noexcept
	: Node{ name }
{}

void Composite::AddChild(std::unique_ptr<Node>&& pNode) noexcept
{
	m_children.emplace_back(std::move(pNode));
}

Sequence::Sequence(const std::string& name) noexcept
	: Composite{ name }
{
	m_nodeType = NodeType::Sequence;
}

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
	: Composite{ name }
{
	m_nodeType = NodeType::Selector;
}

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
	: Node{ name },
	m_decoratorType{ type },
	m_child{ nullptr }
{
	m_nodeType = NodeType::Decorator;
}

void Decorator::AddChild(std::unique_ptr<Node>&& pNode) noexcept
{
	ASSERT(!m_child, "Decorator already has child node assigned.");
	m_child = std::move(pNode);
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