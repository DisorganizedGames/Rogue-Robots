#pragma once
#include <DOGEngine.h>

struct BTDetectPlayerComponent
{
	//ID
};

struct BTAggroComponent
{
	//ID
};

struct BTAttackComponent
{
	//ID
};

struct BTJumpAtPlayerComponent
{
	//ID
};

struct BTPullBackComponent
{
	//ID
};

struct BTDodgeComponent
{
	//ID
};

struct BTMoveToPlayerComponent
{
	//ID
};

struct BTCreatePatrolComponent
{
	//ID
};

struct BTExecutePatrolComponent
{
	//ID
};

struct BTHitDetectComponent
{
	//ID
};

struct BTDistanceToPlayerComponent
{
	//ID
};

struct BTLineOfSightToPlayerComponent
{
	//ID
};

struct BTGetPathComponent
{
	//ID
};

enum class NodeType : uint8_t { Sequence = 0, Selector, Decorator, Leaf };
enum class DecoratorType : uint8_t { Inverter = 0, Succeeder, Failer, Root };

class Node
{
public:
	Node(const std::string& name, NodeType type) noexcept;
	virtual ~Node() noexcept = default;
	virtual void Process(DOG::entity agent) noexcept = 0;
	
	[[nodiscard]] constexpr const std::string& GetName() const noexcept { return m_name; }
	[[nodiscard]] constexpr const NodeType GetNodeType() const noexcept { return m_nodeType; }
	[[nodiscard]] Node* GetParent() const noexcept { return m_parent; }
	[[nodiscard]] constexpr const bool Succeeded() const noexcept { return m_succeeded; }
	void SetSucceededAs(bool result) noexcept { m_succeeded = result; }
	void SetParent(Node* pNode) noexcept;
private:
	bool m_succeeded = false;
	std::string m_name;
	NodeType m_nodeType;
	Node* m_parent;
};

class Composite : public Node
{
public:
	Composite(const std::string& name, NodeType type) noexcept;
	virtual ~Composite() noexcept override = default;
	virtual void Process(DOG::entity agent) noexcept override = 0;
	void Reset() noexcept;
	[[nodiscard]] constexpr const std::vector<std::shared_ptr<Node>>& GetChildren() const noexcept { return m_children; }
	void AddChild(std::shared_ptr<Node>&& pNode) noexcept;
	u32 m_currentChildIndex = 0;
private:
	std::vector<std::shared_ptr<Node>> m_children;
};

class Sequence : public Composite 
{
public:
	Sequence(const std::string& name) noexcept;
	virtual ~Sequence() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
};

class Selector : public Composite
{
public:
	Selector(const std::string& name) noexcept;
	virtual ~Selector() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
};

class Decorator : public Node
{
public:
	Decorator(const std::string& name, const DecoratorType type) noexcept;
	virtual ~Decorator() noexcept override = default;
	virtual void Process(DOG::entity agent) noexcept = 0;
	[[nodiscard]] constexpr const DecoratorType GetDecoratorType() const noexcept { return m_decoratorType; }
	[[nodiscard]] constexpr const std::shared_ptr<Node>& GetChild() const noexcept { return m_child; }
	void AddChild(std::shared_ptr<Node>&& pNode) noexcept;
	bool processed = false;
protected:
	DecoratorType m_decoratorType;
private:
	std::shared_ptr<Node> m_child;
};

class Succeeder : public Decorator
{
public:
	Succeeder(const std::string& name) noexcept;
	virtual ~Succeeder() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
};

class Failer : public Decorator
{
public:
	Failer(const std::string& name) noexcept;
	virtual ~Failer() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
};

class Inverter : public Decorator
{
public:
	Inverter(const std::string& name) noexcept;
	virtual ~Inverter() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
};

class Root : public Decorator
{
public:
	Root(const std::string& name) noexcept;
	virtual ~Root() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
};

class Leaf : public Node
{
public:
	Leaf(const std::string& name) noexcept;
	virtual ~Leaf() noexcept override = default;
	virtual void Process(DOG::entity agent) noexcept override = 0;
	virtual void Succeed(DOG::entity agent) noexcept = 0;
	virtual void Fail(DOG::entity agent) noexcept = 0;
	void ForceSucceed(DOG::entity agent) noexcept;
	void ForceFail(DOG::entity agent) noexcept;
};

class DetectPlayerNode : public Leaf
{
public:
	DetectPlayerNode(const std::string& name) noexcept;
	virtual ~DetectPlayerNode() noexcept = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};

class DetectHitNode : public Leaf 
{
public:
	DetectHitNode(const std::string& name) noexcept;
	virtual ~DetectHitNode() noexcept = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};

class SignalGroupNode : public Leaf
{
public:
	SignalGroupNode(const std::string& name) noexcept;
	virtual ~SignalGroupNode() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};

class AttackNode : public Leaf
{
public:
	AttackNode(const std::string& name) noexcept;
	virtual ~AttackNode() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};

class MoveToPlayerNode : public Leaf
{
public:
	MoveToPlayerNode(const std::string& name) noexcept;
	virtual ~MoveToPlayerNode() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};

class DistanceToPlayerNode : public Leaf 
{
public:
	DistanceToPlayerNode(const std::string& name) noexcept;
	virtual ~DistanceToPlayerNode() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};

class IsAlertNode : public Leaf 
{
public:
	IsAlertNode(const std::string& name) noexcept;
	virtual ~IsAlertNode() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};

class LineOfSightToPlayerNode : public Leaf
{
public:
	LineOfSightToPlayerNode(const std::string& name) noexcept;
	virtual ~LineOfSightToPlayerNode() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};

class GetPathNode : public Leaf
{
public:
	GetPathNode(const std::string& name) noexcept;
	virtual ~GetPathNode() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};


class JumpAtPlayerNode : public Leaf
{
public:
	JumpAtPlayerNode(const std::string& name) noexcept;
	virtual ~JumpAtPlayerNode() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};

class PullBackNode : public Leaf
{
public:
	PullBackNode(const std::string& name) noexcept;
	virtual ~PullBackNode() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};

class DodgeNode : public Leaf
{
public:
	DodgeNode(const std::string& name) noexcept;
	virtual ~DodgeNode() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};








class CreatePatrolNode : public Leaf
{
public:
	CreatePatrolNode(const std::string& name) noexcept;
	virtual ~CreatePatrolNode() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};

class ExecutePatrolNode : public Leaf
{
public:
	ExecutePatrolNode(const std::string& name) noexcept;
	virtual ~ExecutePatrolNode() noexcept override final = default;
	virtual void Process(DOG::entity agent) noexcept override final;
	virtual void Succeed(DOG::entity agent) noexcept override final;
	virtual void Fail(DOG::entity agent) noexcept override final;
};

struct BehaviorTreeComponent
{
	std::unique_ptr<Root> rootNode{ nullptr };
	Node* currentRunningNode{ nullptr };
};

#define LEAF(x) static_cast<Leaf*>(x)

namespace BehaviorTree
{
	void ToGraphViz(Node* root, const std::string filename);
	void ToGraphVizHelper(Node* node, std::ofstream& outstream);
}