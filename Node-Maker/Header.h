#pragma once

struct Planet;

struct TinyCamera
{
	RectF   targetDrawingRegion;        //目標の描画範囲
	RectF   smoothDrawingRegion;        //実際の描画範囲
	double  followingSpeed = 0.2;       //遅延。0.0～1.0

	TinyCamera(RectF _drawingRegion);   //指定した範囲
	TinyCamera();                       //WindowSizeに指定。グローバル変数にしてはいけない。

	void    update();                           //毎フレーム呼ぶ。視点移動を止める場合はこれを使う
	Transformer2D   createTransformer() const;  //視点移動
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