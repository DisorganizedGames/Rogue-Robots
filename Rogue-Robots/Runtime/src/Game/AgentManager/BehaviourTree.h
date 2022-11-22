#pragma once

enum class NodeType : uint8_t { None = 0, Sequence, Selector, Decorator, Leaf };
enum class DecoratorType : uint8_t { None = 0, Inverter, Succeeder, Failer };

class BehaviourTree
{

};

class Node
{
public:
	Node(const std::string& name) noexcept;
	virtual ~Node() noexcept = default;
	virtual bool Process() noexcept = 0;
	[[nodiscard]] constexpr const std::string& GetName() const noexcept { return m_name; }
	[[nodiscard]] constexpr const NodeType GetNodeType() const noexcept { return m_nodeType; }
protected:
	std::string m_name;
	NodeType m_nodeType;
};

class Composite : public Node
{
public:
	Composite(const std::string& name) noexcept;
	virtual ~Composite() noexcept override = default;
	virtual bool Process() noexcept override = 0;
	[[nodiscard]] constexpr const std::list<std::unique_ptr<Node>>& GetChildren() const noexcept { return m_children; }
	void AddChild(std::unique_ptr<Node>&& pNode) noexcept;
private:
	std::list<std::unique_ptr<Node>> m_children;
};

class Sequence : public Composite 
{
public:
	Sequence(const std::string& name) noexcept;
	virtual ~Sequence() noexcept override final = default;
	virtual bool Process() noexcept override final;
};

class Selector : public Composite
{
public:
	Selector(const std::string& name) noexcept;
	virtual ~Selector() noexcept override final = default;
	virtual bool Process() noexcept override final;
};

class Decorator : public Node
{
public:
	Decorator(const std::string& name, const DecoratorType type) noexcept;
	virtual ~Decorator() noexcept override = default;
	virtual bool Process() noexcept = 0;
	[[nodiscard]] constexpr const DecoratorType GetDecoratorType() const noexcept { return m_decoratorType; }
	[[nodiscard]] constexpr const std::unique_ptr<Node>& GetChild() const noexcept { return m_child; }
	void AddChild(std::unique_ptr<Node>&& pNode) noexcept;
protected:
	DecoratorType m_decoratorType;
private:
	std::unique_ptr<Node> m_child;
};

class Succeeder : public Decorator
{
public:
	Succeeder(const std::string& name) noexcept;
	virtual ~Succeeder() noexcept override final = default;
	virtual bool Process() noexcept override final;
};

class Failer : public Decorator
{
public:
	Failer(const std::string& name) noexcept;
	virtual ~Failer() noexcept override final = default;
	virtual bool Process() noexcept override final;
};

class Inverter : public Decorator
{
public:
	Inverter(const std::string& name) noexcept;
	virtual ~Inverter() noexcept override final = default;
	virtual bool Process() noexcept override final;
};