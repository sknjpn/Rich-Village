#pragma once

struct Planet;

struct TinyCamera
{
	RectF   targetDrawingRegion;        //�ڕW�̕`��͈�
	RectF   smoothDrawingRegion;        //���ۂ̕`��͈�
	double  followingSpeed = 0.2;       //�x���B0.0�`1.0

	TinyCamera(RectF _drawingRegion);   //�w�肵���͈�
	TinyCamera();                       //WindowSize�Ɏw��B�O���[�o���ϐ��ɂ��Ă͂����Ȃ��B

	void    update();                           //���t���[���ĂԁB���_�ړ����~�߂�ꍇ�͂�����g��
	Transformer2D   createTransformer() const;  //���_�ړ�
};

struct BData
{
	String	name;
	double	size;
	Color	color;
	Planet*	planet;

	BData(JSONValue j, Planet* _planet)
		: name(j[L"name"].getString())
		, size(j[L"size"].get<double>())
		, color(j[L"color"].get<Color>())
		, planet(_planet)
	{}

	int		id() const;
};

struct Node
{
	bool	enabled;
	Vec2	pos;
	BData*	bData;
	Array<Node*>	connections;
	Planet*	planet;

	Node(Planet* _planet)
		: planet(_planet)
		, pos(0.0, 0.0)
		, bData(nullptr)
	{}

	void	remove();
	int		id() const;
	Circle	circle() const;
};

enum struct PMode
{
	None,
	MakeNode,
	BreakNode,
	MoveNode,
	ConnectNode,
};

struct Planet
{
	Array<BData>	bData;

	Array<Node>	nodes;

	TinyCamera tinyCamera;
	PMode	pMode;

	Node*	grabedNode = nullptr;
	BData*	selectedBData;

	Planet();

	void	save(const FilePath& _path);
	void	load(const FilePath& _path);

	bool	canSetNode(const Vec2& _pos, BData* _bData) const;
	void	setNode(const Vec2& _pos, BData* _bData);
	Node*	newNode();
	Node*	mouseOveredNode();
};