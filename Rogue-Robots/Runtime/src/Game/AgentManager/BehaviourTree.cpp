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

void Composite::Reset() noexcept
{
	m_currentChildIndex = 0u;
}

Sequence::Sequence(const std::string& name) noexcept
	: Composite{ name, NodeType::Sequence }
{}

void Sequence::Process(DOG::entity agent) noexcept
{
	//Are we at the very first child?
	const bool sequenceIsAtTheVeryStart = (m_currentChildIndex == 0u);
	if (sequenceIsAtTheVeryStart)
	{
		auto& pCurrentChild = GetChildren()[m_currentChildIndex];
		//We are, so we should just process it:
		pCurrentChild->Process(agent);
	}
	else
	{
		//We are not at the very first child. 
		//If the previous child failed we should mark the sequence as a failure:
		auto& pCompletedChild = GetChildren()[m_currentChildIndex - 1];
		if (!pCompletedChild->Succeeded())
		{
			Reset();
			SetSucceededAs(false);
			return;
		}

		//By this point we know the previous child succeeded. Have we processed all children?
		//If so, we should mark the sequence as succeeded and return:
		const bool doneProcessing = (m_currentChildIndex == GetChildren().size());
		if (doneProcessing)
		{
			Reset();
			SetSucceededAs(true);
			return;
		}

		//At this point we know that we should just continue going:
		auto& pCurrentChild = GetChildren()[m_currentChildIndex];
		pCurrentChild->Process(agent);
	}
	m_currentChildIndex++;
}

Selector::Selector(const std::string& name) noexcept
	: Composite{ name, NodeType::Selector }
{}

void Selector::Process(DOG::entity agent) noexcept
{
	//Are we at the very first child?
	const bool sequenceIsAtTheVeryStart = (m_currentChildIndex == 0u);
	if (sequenceIsAtTheVeryStart)
	{
		auto& pCurrentChild = GetChildren()[m_currentChildIndex];
		//We are, so we should just process it:
		pCurrentChild->Process(agent);
	}
	else
	{
		//We are not at the very first child. 
		//If the previous child succeeded we should mark the selector as a success:
		auto& pCompletedChild = GetChildren()[m_currentChildIndex - 1];
		if (pCompletedChild->Succeeded())
		{
			Reset();
			SetSucceededAs(true);
			return;
		}

		//By this point we know the previous child failed. Have we processed all children?
		//If so, we should mark the selector as a failure and return:
		const bool doneProcessing = (m_currentChildIndex == GetChildren().size());
		if (doneProcessing)
		{
			Reset();
			SetSucceededAs(false);
			return;
		}

		//At this point we know that we should just continue going:
		auto& pCurrentChild = GetChildren()[m_currentChildIndex];
		pCurrentChild->Process(agent);
	}
	m_currentChildIndex++;
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

void Succeeder::Process(DOG::entity agent) noexcept
{
	GetChild()->Process(agent);
}

Failer::Failer(const std::string& name) noexcept
	: Decorator{ name, DecoratorType::Failer}
{}

void Failer::Process(DOG::entity agent) noexcept
{
	GetChild()->Process(agent);
	SetSucceededAs(false);
}

Inverter::Inverter(const std::string& name) noexcept
	: Decorator{ name, DecoratorType::Inverter}
{}

void Inverter::Process(DOG::entity agent) noexcept
{
	GetChild()->Process(agent);
}

Root::Root(const std::string& name) noexcept
	: Decorator(name, DecoratorType::Root)
{}

void Root::Process(DOG::entity agent) noexcept
{
	GetChild()->Process(agent);
}

Leaf::Leaf(const std::string& name) noexcept
	: Node{ name, NodeType::Leaf }
{}

DetectPlayerNode::DetectPlayerNode(const std::string& name) noexcept
	: Leaf{ name }
{}

void DetectPlayerNode::Process(DOG::entity agent) noexcept
{
	DOG::EntityManager::Get().AddComponent<DetectPlayerComponent>(agent);
	DOG::EntityManager::Get().GetComponent<BehaviourTreeComponent>(agent).currentRunningNode = this;
}

void DetectPlayerNode::Succeed(DOG::entity agent) noexcept
{
	SetSucceededAs(true);
	DOG::EntityManager::Get().RemoveComponent<DetectPlayerComponent>(agent);
	GetParent()->Process(agent);
}

void DetectPlayerNode::Fail(DOG::entity agent) noexcept
{
	SetSucceededAs(false);
	DOG::EntityManager::Get().RemoveComponent<DetectPlayerComponent>(agent);
	GetParent()->Process(agent);
}

SignalGroupNode::SignalGroupNode(const std::string& name) noexcept
	: Leaf{ name }
{}

void SignalGroupNode::Process(DOG::entity agent) noexcept
{
	DOG::EntityManager::Get().AddComponent<SignalGroupComponent>(agent);
	DOG::EntityManager::Get().GetComponent<BehaviourTreeComponent>(agent).currentRunningNode = this;
}

void SignalGroupNode::Succeed(DOG::entity agent) noexcept
{
	SetSucceededAs(true);
	DOG::EntityManager::Get().RemoveComponent<SignalGroupComponent>(agent);
	GetParent()->Process(agent);
}

void SignalGroupNode::Fail(DOG::entity agent) noexcept
{
	SetSucceededAs(false);
	DOG::EntityManager::Get().RemoveComponent<SignalGroupComponent>(agent);
	GetParent()->Process(agent);
}