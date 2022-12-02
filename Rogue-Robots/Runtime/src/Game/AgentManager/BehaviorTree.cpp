#include "BehaviorTree.h"
#include "AgentComponents.h"

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
		m_currentChildIndex++;
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
			GetParent()->Process(agent);
			return;
		}

		//By this point we know the previous child succeeded. Have we processed all children?
		//If so, we should mark the sequence as succeeded and return:
		const bool doneProcessing = (m_currentChildIndex == GetChildren().size());
		if (doneProcessing)
		{
			Reset();
			SetSucceededAs(true);
			GetParent()->Process(agent);
			return;
		}

		//At this point we know that we should just continue going:
		auto& pCurrentChild = GetChildren()[m_currentChildIndex];
		m_currentChildIndex++;
		pCurrentChild->Process(agent);
	}
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
		m_currentChildIndex++;
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
			GetParent()->Process(agent);
			return;
		}

		//By this point we know the previous child failed. Have we processed all children?
		//If so, we should mark the selector as a failure and return:
		const bool doneProcessing = (m_currentChildIndex == GetChildren().size());
		if (doneProcessing)
		{
			Reset();
			SetSucceededAs(false);
			GetParent()->Process(agent);
			return;
		}

		//At this point we know that we should just continue going:
		auto& pCurrentChild = GetChildren()[m_currentChildIndex];
		m_currentChildIndex++;
		pCurrentChild->Process(agent);
	}
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
	if (!processed)
	{
		processed = true;
		GetChild()->Process(agent);
	}
	else
	{
		SetSucceededAs(true);
		processed = false;
		GetParent()->Process(agent);
	}
}

Failer::Failer(const std::string& name) noexcept
	: Decorator{ name, DecoratorType::Failer}
{}

void Failer::Process(DOG::entity agent) noexcept
{
	if (!processed)
	{
		processed = true;
		GetChild()->Process(agent);
	}
	else
	{
		processed = false;
		SetSucceededAs(false);
		GetParent()->Process(agent);
	}
}

Inverter::Inverter(const std::string& name) noexcept
	: Decorator{ name, DecoratorType::Inverter}
{}

void Inverter::Process(DOG::entity agent) noexcept
{
	if (!processed)
	{
		processed = true;
		GetChild()->Process(agent);
	}
	else
	{
		if (GetChild()->Succeeded())
		{
			SetSucceededAs(false);
		}
		else
		{
			SetSucceededAs(true);
		}
		processed = false;
		GetParent()->Process(agent);
	}
}

Root::Root(const std::string& name) noexcept
	: Decorator(name, DecoratorType::Root)
{}

void Root::Process(DOG::entity agent) noexcept
{
	if (!processed)
	{
		processed = true;
		GetChild()->Process(agent);
	}
	else
	{
		processed = false;
		DOG::EntityManager::Get().GetComponent<BehaviorTreeComponent>(agent).currentRunningNode = this;
	}
}

Leaf::Leaf(const std::string& name) noexcept
	: Node{ name, NodeType::Leaf }
{}

void Leaf::ForceSucceed(DOG::entity agent) noexcept
{
	SetSucceededAs(true);
	GetParent()->Process(agent);
}

void Leaf::ForceFail(DOG::entity agent) noexcept
{
	SetSucceededAs(false);
	GetParent()->Process(agent);
}

DetectPlayerNode::DetectPlayerNode(const std::string& name) noexcept
	: Leaf{ name }
{}

void DetectPlayerNode::Process(DOG::entity agent) noexcept
{
	//bool forceSucced = DOG::EntityManager::Get().HasComponent<BTAttackComponent>(agent) || DOG::EntityManager::Get().HasComponent<AgentAggroComponent>(agent);

	if (!DOG::EntityManager::Get().HasComponent<BTAttackComponent>(agent))
	{
		DOG::EntityManager::Get().AddComponent<BTDetectPlayerComponent>(agent);
		DOG::EntityManager::Get().GetComponent<BehaviorTreeComponent>(agent).currentRunningNode = this;
	}
	else
	{
		ForceSucceed(agent);
	}
}

void DetectPlayerNode::Succeed(DOG::entity agent) noexcept
{
	SetSucceededAs(true);
	DOG::EntityManager::Get().RemoveComponent<BTDetectPlayerComponent>(agent);
	if (!DOG::EntityManager::Get().HasComponent<AgentAggroComponent>(agent))
		DOG::EntityManager::Get().AddComponent<AgentAggroComponent>(agent);

	GetParent()->Process(agent);
}

void DetectPlayerNode::Fail(DOG::entity agent) noexcept
{
	SetSucceededAs(false);
	DOG::EntityManager::Get().RemoveComponent<BTDetectPlayerComponent>(agent);
	DOG::EntityManager::Get().RemoveComponentIfExists<AgentAggroComponent>(agent);
	GetParent()->Process(agent);
}

DetectHitNode::DetectHitNode(const std::string& name) noexcept
	: Leaf{ name }
{}

void DetectHitNode::Process(DOG::entity agent) noexcept
{
	if (DOG::EntityManager::Get().HasComponent<DOG::HasEnteredCollisionComponent>(agent))
	{
		DOG::EntityManager::Get().AddComponent<BTHitDetectComponent>(agent);
		DOG::EntityManager::Get().GetComponent<BehaviorTreeComponent>(agent).currentRunningNode = this;
	}
	else
	{
		ForceFail(agent); // Go to next node of SELECTOR, which is DetectPlayer as of now.
	}
}

void DetectHitNode::Succeed(DOG::entity agent) noexcept
{
	SetSucceededAs(true);
	DOG::EntityManager::Get().RemoveComponent<BTHitDetectComponent>(agent);
	if (!DOG::EntityManager::Get().HasComponent<AgentAggroComponent>(agent))
		DOG::EntityManager::Get().AddComponent<AgentAggroComponent>(agent);

	GetParent()->Process(agent);
}

void DetectHitNode::Fail(DOG::entity agent) noexcept
{
	SetSucceededAs(false);
	DOG::EntityManager::Get().RemoveComponent<BTHitDetectComponent>(agent);
	DOG::EntityManager::Get().RemoveComponentIfExists<AgentAggroComponent>(agent);
	GetParent()->Process(agent);
}

SignalGroupNode::SignalGroupNode(const std::string& name) noexcept
	: Leaf{ name }
{}

void SignalGroupNode::Process(DOG::entity agent) noexcept
{
	if (!DOG::EntityManager::Get().HasComponent<BTAttackComponent>(agent))
	{
		DOG::EntityManager::Get().AddComponent<BTAggroComponent>(agent);
		DOG::EntityManager::Get().GetComponent<BehaviorTreeComponent>(agent).currentRunningNode = this;
	}
	else
	{
		ForceSucceed(agent);
	}
}

void SignalGroupNode::Succeed(DOG::entity agent) noexcept
{
	SetSucceededAs(true);
	DOG::EntityManager::Get().RemoveComponent<BTAggroComponent>(agent);
	GetParent()->Process(agent);
}

void SignalGroupNode::Fail(DOG::entity agent) noexcept
{
	SetSucceededAs(false);
	DOG::EntityManager::Get().RemoveComponent<BTAggroComponent>(agent);
	GetParent()->Process(agent);
}

AttackNode::AttackNode(const std::string& name) noexcept
	: Leaf{ name }
{}

void AttackNode::Process(DOG::entity agent) noexcept
{
	DOG::EntityManager::Get().AddOrReplaceComponent<BTAttackComponent>(agent);
	DOG::EntityManager::Get().GetComponent<BehaviorTreeComponent>(agent).currentRunningNode = this;
}

void AttackNode::Succeed(DOG::entity agent) noexcept
{
	SetSucceededAs(true);
	DOG::EntityManager::Get().RemoveComponent<BTAttackComponent>(agent);
	GetParent()->Process(agent);
}

void AttackNode::Fail(DOG::entity agent) noexcept
{
	SetSucceededAs(false);
	DOG::EntityManager::Get().RemoveComponent<BTAttackComponent>(agent);
	GetParent()->Process(agent);
}

MoveToPlayerNode::MoveToPlayerNode(const std::string& name) noexcept
	: Leaf{ name }
{}

void MoveToPlayerNode::Process(DOG::entity agent) noexcept
{
	DOG::EntityManager::Get().AddComponent<BTMoveToPlayerComponent>(agent);
	DOG::EntityManager::Get().GetComponent<BehaviorTreeComponent>(agent).currentRunningNode = this;
}

void MoveToPlayerNode::Succeed(DOG::entity agent) noexcept
{
	SetSucceededAs(true);
	DOG::EntityManager::Get().RemoveComponent<BTMoveToPlayerComponent>(agent);
	GetParent()->Process(agent);
}

void MoveToPlayerNode::Fail(DOG::entity agent) noexcept
{
	SetSucceededAs(false);
	DOG::EntityManager::Get().RemoveComponent<BTMoveToPlayerComponent>(agent);
	GetParent()->Process(agent);
}

PatrolNode::PatrolNode(const std::string& name) noexcept
	: Leaf{ name }
{}

void PatrolNode::Process(DOG::entity agent) noexcept
{
	DOG::EntityManager::Get().AddComponent<BTPatrolComponent>(agent);
	DOG::EntityManager::Get().GetComponent<BehaviorTreeComponent>(agent).currentRunningNode = this;
}

void PatrolNode::Succeed(DOG::entity agent) noexcept
{
	SetSucceededAs(true);
	DOG::EntityManager::Get().RemoveComponent<BTPatrolComponent>(agent);
	GetParent()->Process(agent);
}

void PatrolNode::Fail(DOG::entity agent) noexcept
{
	SetSucceededAs(false);
	DOG::EntityManager::Get().RemoveComponent<BTPatrolComponent>(agent);
	GetParent()->Process(agent);
}

void BehaviorTree::ToGraphViz(Node* root)
{
	std::ofstream outStream;
	outStream.open("BehaviorTree.dot");

	outStream << "digraph {" << std::endl;
	ToGraphVizHelper(root, outStream);
	outStream << "}" << std::endl;
	outStream.close();
}

void BehaviorTree::ToGraphVizHelper(Node* node, std::ofstream& outstream)
{
	outstream << node->GetName() << " [label=\"" << node->GetName() << "\\n";

	switch (node->GetNodeType())
	{
	case NodeType::Sequence:
		outstream << "[Sequence]\" shape=box";
		break;
	case NodeType::Selector:
		outstream << "[Selector]\" shape=box";
		break;
	case NodeType::Decorator:
	{
		switch (static_cast<Decorator*>(node)->GetDecoratorType())
		{
		case DecoratorType::Inverter:
			outstream << "[Inverter]\" shape=box";
			break;
		case DecoratorType::Succeeder:
			outstream << "[Succeeder]\" shape=box";
			break;
		case DecoratorType::Failer:
			outstream << "[Failer]\" shape=box";
			break;
		case DecoratorType::Root:
			outstream << "[Root]\" shape=invhouse";
			break;
		default:
			outstream << "[undefined]\"";
			break;
		}
	}
		break;
	case NodeType::Leaf:
		outstream << "[Leaf]\"";
		break;
	default:
		break;
	}

	outstream << "];" << std::endl;

	switch (node->GetNodeType())
	{
	case NodeType::Sequence:
	case NodeType::Selector:
		for (auto& child : static_cast<Composite*>(node)->GetChildren())
		{
			ToGraphVizHelper(child.get(), outstream);
			outstream << node->GetName() << " -> " << child->GetName() << ";" << std::endl;
		}
		break;
	case NodeType::Decorator:
		{
			Node* child = static_cast<Decorator*>(node)->GetChild().get();
			ToGraphVizHelper(child, outstream);
			outstream << node->GetName() << " -> " << child->GetName() << ";" << std::endl;
		}
		break;
	default:
		break;
	}
}