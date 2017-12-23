#pragma once

struct Planet;
struct Node;

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

	BData(JSONValue j, Planet* _planet);

	int		id() const;
};

struct IData
{
	String	name;
	int		food;
	int		clothes;
	Texture	texture;
	Planet*	planet;

	IData(JSONValue j, Planet* _planet);
};

struct Work
{
	int		needCost;
	int		earnGold;
	Array<std::pair<IData*, int>>	needItems;
	Array<std::pair<IData*, int>>	earnItems;
	Planet*	planet;

	Work(JSONValue j, Planet* _planet);
};

struct FData
{
	String	name;
	Color	color;
	int		price;
	int		maxWork;
	int		maxLive;
	bool	hasWork;
	Work	work;
	BData*	biome;
	Planet*	planet;

	FData(JSONValue j, Planet* _planet);
};

struct Facility
{
	bool	enabled;
	int		labour;
	Node*	node;
	int		numWork;
	int		numLive;
	double	wage;
	double	rent;
	FData*	fData;
	Array<std::pair<IData*, int>>	items;
	Planet*	planet;

	Facility(Node* _node, Planet* _planet);

	int		numItem(IData* _iData);
	bool	isProduced(IData* _iData);
	void	addItem(IData* _iData, int _num = 1);
	void	pullItem(IData* _iData, int _num = 1);
	void	addLabour();
};

struct Node
{
	//一時変数
	Node*	from;
	double	cost;

	Vec2	pos;
	BData*	bData;
	Facility	facitity;
	Array<Node*>	connections;
	Planet*	planet;

	Node(Planet* _planet);

	Array<Node*>	getRotueTo(Node* _to);
	double	getLengthTo(Node* _to);
	bool	hasFacility() const;
	int		id() const;
	bool	mouseOver() const;
	Circle	circle() const;
};

enum struct UMode
{
	None,
	Work,
	GoToWork,
	GoToHome,
	TakeNeedItems,
	TakeRest,
	TakeFood,
	TakeClothes,
};

struct Unit
{
	//ステータス
	int		labour;	//疲労
	int		hungry;	//空腹
	int		clothes;//衣類

	bool	enabled;
	String	name;
	Color	color;	//識別用
	Vec2	delta;	//Node内変異
	double	z;		//上方向
	Node*	nowNode;//現在のNode
	double	t;		//routeのfrontの進行度
	UMode	uMode;
	Node*	workedNode;
	Node*	livedNode;
	Array<Node*>	route;
	Array<std::pair<IData*, int>>	items;
	Planet*	planet;

	Unit(Planet* _planet);

	void	update();
	void	draw();
	void	addItem(IData* _iData, int _num = 1);
	bool	moveTo(IData* _iData, int _num = 1);
	bool	moveToFood();
	bool	moveToClothes();
	void	moveTo(Node* _to);

	Vec2	pos() const;
	bool	mouseOver() const;
	Circle	circle() const;

	void	setWorkedNode(Node* _node);
	void	setLivedNode(Node* _node);
	Node*	searchBestLivedNode();
};

struct Planet
{
	Array<BData>	bData;
	Array<IData>	iData;
	Array<FData>	fData;

	Array<Node>	nodes;
	Array<Unit>	units;

	Font	mFont;

	int		time;
	int		money;

	Unit*	selectedUnit;
	FData*	selectedFData;

	TinyCamera tinyCamera;

	Planet();

	void	load(const FilePath& _path);

	Node*	mouseOveredNode();
	Unit*	mouseOveredUnit();
	BData*	getBData(const String& _name);
	IData*	getIData(const String& _name);
	Unit*	getNewUnit();

	double	monthProgress();
	int		month();
	int		year();
};