# include <Siv3D.hpp> // OpenSiv3D v0.1.7
# include "Header.h"

void Main()
{
	Planet planet;
}

Planet::Planet()
	: mFont(16, Typeface::Bold, FontStyle::Default)
	, selectedFData(nullptr)
	, selectedUnit(nullptr)
{
	const auto nodeLineThickness = 32.0;
	const auto nodeLineColor = Palette::Khaki;

	Font fFont(20, Typeface::Bold, FontStyle::Default);
	Font iFont(16, Typeface::Default, FontStyle::Default);

	//BData
	{
		JSONReader json(L"assets/biomes.json");
		for (auto j : json.arrayView()) bData.emplace_back(j, this);
	}

	//IData
	{
		JSONReader json(L"assets/items.json");
		for (auto j : json.arrayView()) iData.emplace_back(j, this);
	}

	//FData
	{
		JSONReader json(L"assets/facilities.json");
		for (auto j : json.arrayView()) fData.emplace_back(j, this);
	}

	money = 10000;
	time = 0;

	load(L"assets/map.bin");

	Window::Resize(1280, 720);
	Window::SetTitle(L"Road of Gold 2");

	Graphics::SetBackground(Palette::Darkkhaki);

	units.reserve(4096);

	for (int i = 0; i < 128; i++)
	{
		auto& u = units.emplace_back(this);
		u.name = L"hoge";
		u.color = RandomHSV();
		u.enabled = true;
		u.delta = Vec2::Zero();
		u.z = 0.0;
		u.nowNode = &nodes[Random(nodes.size() - 1)];
		u.t = 0.0;
	}

	for (auto& n : nodes)
	{
		if (RandomBool(1.0)) continue;
		auto& f = n.facitity;
		f.enabled = true;
		while (f.fData == nullptr || f.fData->biome != n.bData)
		{
			f.fData = &fData[Random(fData.size() - 1)];
		}
	}

	while (System::Update())
	{
		tinyCamera.update();

		//Unit処理
		for (int i = 0; i < (KeyShift.pressed() ? 100 : 1); i++)
		{
			//時間
			time++;

			for (auto& u : units) u.update();
		}
		{
			auto tc = tinyCamera.createTransformer();

			//Node接続の描画
			for (auto& n : nodes)
			{
				for (auto* cn : n.connections) Line(n.pos, cn->pos).draw(nodeLineThickness, nodeLineColor);
			}

			//Node描画
			for (auto& n : nodes)
			{
				n.circle().draw(n.bData->color);
				if (n.circle().mouseOver()) n.circle().draw(ColorF(1.0, 0.2)).drawFrame(16.0, Palette::Red);
			}

			//UnitLine描画
			for (auto& u : units)
			{
				if (!u.route.isEmpty())
				{
					LineString ls;
					ls.emplace_back(u.nowNode->pos.lerp(u.route.front()->pos, u.t));
					for (auto* n : u.route) ls.emplace_back(n->pos);
					ls.drawCatmullRom(nodeLineThickness / 2.0, ColorF(u.color, 0.5));
				}
			}

			//FacilityCircle描画
			for (auto& n : nodes)
			{
				if (n.facitity.enabled) Circle(n.pos, 80).draw(n.facitity.fData->color);
			}

			//Unit描画
			for (auto& u : units) u.draw();

			//Facility描画
			for (auto& n : nodes)
			{
				auto& f = n.facitity;

				if (!f.enabled) continue;

				fFont(f.fData->name).drawAt(n.pos, Palette::Black);

				if (f.fData->maxWork > 0)
				{
					auto fw = iFont(L"賃金", f.wage, L"G ", f.numWork, L"/", f.fData->maxWork);
					auto rw = fw.region().movedBy(n.pos + Vec2(-48, -40));
					if (rw.mouseOver()) rw.draw(ColorF(1.0, 0.5));
					if (rw.leftClicked()) f.wage += KeyShift.pressed() ? 10 : 1;
					if (rw.rightClicked()) f.wage -= KeyShift.pressed() ? 10 : 1;
					fw.draw(rw.pos, Palette::Black);
				}

				if (f.fData->maxLive > 0)
				{
					auto fr = iFont(L"家賃", f.rent, L"G ", f.numLive, L"/", f.fData->maxLive);
					auto rr = fr.region().movedBy(n.pos + Vec2(-48, -40));
					if (rr.mouseOver()) rr.draw(ColorF(1.0, 0.5));
					if (rr.leftClicked()) f.rent += KeyShift.pressed() ? 10 : 1;
					if (rr.rightClicked()) f.rent -= KeyShift.pressed() ? 10 : 1;
					fr.draw(rr.pos, Palette::Black);
				}

				for (int i = 0; i < int(f.items.size()); i++)
				{
					auto& item = f.items[i];
					item.first->texture.resize(16, 16).draw(n.pos.movedBy(-36, 12 + i * 16));
					iFont(L"x", item.second).draw(n.pos.movedBy(-20, 8 + i * 16), Palette::Black);
				}
			}

			//selectedFData
			if (selectedFData != nullptr)
			{
				selectedUnit = nullptr;

				auto* tar = mouseOveredNode();

				if (tar == nullptr || tar->facitity.enabled || tar->bData != selectedFData->biome)
				{
					Circle(Cursor::PosF(), 80.0).draw(selectedFData->color);
				}
				else
				{
					Circle(tar->pos, 80.0).draw(selectedFData->color);
					if (MouseL.down())
					{
						money -= selectedFData->price;
						tar->facitity.enabled = true;
						tar->facitity.fData = selectedFData;
					}
				}
			}
			else
			{
				auto* u = mouseOveredUnit();

				if (u != nullptr && MouseL.down()) selectedUnit = u;

				if (selectedUnit != nullptr)
				{
					tinyCamera.targetDrawingRegion.setCenter(selectedUnit->pos());
				}
			}
		}

		if (MouseR.down())
		{
			selectedFData = nullptr;
			selectedUnit = nullptr;
		}

		//UI描画
		{
			//お金
			{
				Rect rect(Window::Size().x - 160, 0, 160, 32);
				rect.draw(ColorF(0.3));
				fFont(L"資産", money, L"G").drawAt(rect.center());
			}

			//時間
			{
				Rect rect(160, 32);
				rect.draw(ColorF(0.3)).drawFrame(2.0, ColorF(0.1));
				RectF(rect).setSize(rect.size.x*monthProgress(), rect.size.y).draw(ColorF(0.5));
				fFont(year(), L"年", month(), L"月").drawAt(rect.center());
			}

			//建物
			{

				for (int i = 0; i < int(fData.size()); i++)
				{
					auto& fd = fData[i];
					Rect rect(0, 32 + 32 * i, 160, 32);
					rect.draw(ColorF((selectedFData == &fd) ? 0.6 : rect.mouseOver() ? 0.4 : 0.2)).drawFrame(2.0, ColorF(0.1));

					if (rect.leftClicked())
					{
						if (selectedFData == &fd) selectedFData = nullptr;
						else selectedFData = &fd;
					}

					fFont(fd.name).drawAt(rect.center());
				}
			}
		}
	}
}

double	Planet::monthProgress()
{
	return (time % 3600) / 3600.0;
}

int		Planet::month()
{
	return (time / 3600) % 12 + 1;
}

int		Planet::year()
{
	return time / 3600 / 12 + 1;
}

bool	Unit::moveTo(IData* _iData, int _num)
{
	double max = 0.0;
	Node* tar = nullptr;

	for (auto& n : planet->nodes)
	{
		auto& nf = n.facitity;
		if (nf.enabled && nf.isProduced(_iData) && nf.numItem(_iData) >= _num && (max == 0.0 || max > nowNode->getLengthTo(&n)))
		{
			max = nowNode->getLengthTo(&n);
			tar = &n;
		}
	}

	if (tar != nullptr)
	{
		moveTo(tar);
		return true;
	}
	else
	{
		return false;
	}
}

bool	Unit::moveToFood()
{
	double max = 0.0;
	Node* tar = nullptr;

	for (auto& id : planet->iData)
	{
		if (id.food == 0) continue;

		for (auto& n : planet->nodes)
		{
			auto& nf = n.facitity;
			if (nf.enabled && nf.isProduced(&id) && nf.numItem(&id) > 0 && (max == 0.0 || max > nowNode->getLengthTo(&n)))
			{
				max = nowNode->getLengthTo(&n);
				tar = &n;
			}
		}
	}

	if (tar != nullptr)
	{
		moveTo(tar);
		return true;
	}
	else
	{
		return false;
	}
}

bool	Unit::moveToClothes()
{
	double max = 0.0;
	Node* tar = nullptr;

	for (auto& id : planet->iData)
	{
		if (id.clothes == 0) continue;

		for (auto& n : planet->nodes)
		{
			auto& nf = n.facitity;
			if (nf.enabled && nf.isProduced(&id) && nf.numItem(&id) > 0 && (max == 0.0 || max > nowNode->getLengthTo(&n)))
			{
				max = nowNode->getLengthTo(&n);
				tar = &n;
			}
		}
	}

	if (tar != nullptr)
	{
		moveTo(tar);
		return true;
	}
	else
	{
		return false;
	}
}

BData::BData(JSONValue j, Planet* _planet)
	: name(j[L"name"].getString())
	, size(j[L"size"].get<double>())
	, color(j[L"color"].get<Color>())
	, planet(_planet)
{}

IData::IData(JSONValue j, Planet* _planet)
	: name(j[L"name"].getString())
	, food(j[L"food"].getOr<int>(0))
	, clothes(j[L"clothes"].getOr<int>(0))
	, texture(j[L"texture"].getString())
	, planet(_planet)
{}

Work::Work(JSONValue j, Planet* _planet)
	: needCost(j[L"needCost"].getOr<int>(0))
	, earnGold(j[L"earnGold"].getOr<int>(0))
{
	for (auto o : j[L"needItems"].objectView()) needItems.emplace_back(_planet->getIData(o.name), o.value.get<int>());
	for (auto o : j[L"earnItems"].objectView()) earnItems.emplace_back(_planet->getIData(o.name), o.value.get<int>());
}

FData::FData(JSONValue j, Planet* _planet)
	: name(j[L"name"].getString())
	, color(j[L"color"].get<Color>())
	, price(j[L"price"].get<int>())
	, planet(_planet)
	, biome(_planet->getBData(j[L"biome"].getString()))
	, work(j[L"work"], _planet)
	, hasWork(!j[L"work"].isEmpty())
	, maxLive(j[L"maxLive"].getOr<int>(0))
	, maxWork(j[L"maxWork"].getOr<int>(0))
{}

Facility::Facility(Node* _node, Planet* _planet)
	: labour(0)
	, enabled(false)
	, planet(_planet)
	, node(_node)
	, fData(nullptr)
	, wage(100)
	, rent(100)
	, numWork(0)
	, numLive(0)
{}

Node::Node(Planet* _planet)
	: planet(_planet)
	, pos(0.0, 0.0)
	, bData(nullptr)
	, facitity(this, _planet)
	, cost(0.0)
	, from(nullptr)
{}

Unit::Unit(Planet* _planet)
	: labour(6000)
	, hungry(12000)
	, clothes(36000)
	, enabled(false)
	, planet(_planet)
	, nowNode(nullptr)
	, workedNode(nullptr)
	, livedNode(nullptr)
	, t(0.0)
	, uMode(UMode::None)
{}

void	Facility::addLabour()
{
	if (!enabled) return;

	for (auto& ni : fData->work.needItems)
	{
		if (numItem(ni.first) < ni.second) return;
	}

	labour++;

	if (labour >= fData->work.needCost)
	{
		for (auto& ni : fData->work.needItems) pullItem(ni.first, ni.second);
		for (auto& ei : fData->work.earnItems) addItem(ei.first, ei.second);

		labour -= fData->work.needCost;

		planet->money += fData->work.earnGold;
	}
}

int	Facility::numItem(IData* _iData)
{
	for (auto& item : items)
	{
		if (item.first == _iData) return item.second;
	}

	return 0;
}

bool	Facility::isProduced(IData* _iData)
{
	for (auto& ei : fData->work.earnItems)
	{
		if (ei.first == _iData) return true;
	}
	return false;
}

void	Facility::pullItem(IData* _iData, int _num)
{
	for (auto& item : items)
	{
		if (item.first == _iData)
		{
			item.second -= _num;
		}
	}
	items.remove_if([](std::pair<IData*, int> _i) { return _i.second <= 0; });
}

void	Unit::update()
{
	if (!enabled) return;
	double speed = 0.1;

	//職場の登録
	if (workedNode == nullptr)
	{
		auto* n = &planet->nodes[Random(planet->nodes.size() - 1)];
		auto& f = n->facitity;
		if (f.enabled && f.fData->maxWork - f.numWork > 0)
		{
			setWorkedNode(n);
		}
	}

	//自宅の登録
	if (RandomBool(0.001))
	{
		if (livedNode != nullptr)
		{
			livedNode->facitity.numLive--;
			livedNode = nullptr;
		}
		auto* ln = searchBestLivedNode();
		if (ln != nullptr) setLivedNode(ln);
	}

	//おなかの減り
	hungry = Max(0, hungry - 1);
	clothes = Max(0, clothes - 1);

	if (route.isEmpty())
	{
		if (hungry == 0)
		{
			if (uMode == UMode::TakeFood)
			{
				auto& f = nowNode->facitity;

				for (auto& id : planet->iData)
				{
					if (id.food > 0 && f.numItem(&id) > 0)
					{
						hungry += id.food;
						f.pullItem(&id);
					}
				}

				uMode = UMode::None;
			}
			else if (moveToFood()) uMode = UMode::TakeFood;
		}
		else if (clothes == 0)
		{
			if (uMode == UMode::TakeClothes)
			{
				auto& f = nowNode->facitity;

				for (auto& id : planet->iData)
				{
					if (id.clothes > 0 && f.numItem(&id) > 0)
					{
						clothes += id.clothes;
						f.pullItem(&id);
					}
				}

				uMode = UMode::None;
			}
			else if (moveToClothes()) uMode = UMode::TakeClothes;
		}
		else if (nowNode == livedNode)
		{
			uMode = UMode::TakeRest;
			labour++;
			if (labour >= 600)
			{
				labour = 600;
				if (workedNode != nullptr)
				{
					moveTo(workedNode);
					uMode = UMode::GoToWork;
				}
			}
		}
		else if (nowNode == workedNode)
		{
			uMode = UMode::Work;

			if (!items.isEmpty())
			{
				for (auto& item : items) nowNode->facitity.addItem(item.first, item.second);
			}
			items.clear();

			bool canWork = true;
			auto& f = workedNode->facitity;
			for (auto& ni : f.fData->work.needItems)
			{
				if (f.numItem(ni.first) < ni.second)
				{
					uMode = UMode::TakeNeedItems;
					moveTo(ni.first);
					canWork = false;
				}
			}

			if (canWork)
			{
				f.addLabour();
				labour--;
				if (labour <= 0)
				{
					uMode = UMode::TakeRest;
					if (livedNode != nullptr)
					{
						moveTo(livedNode);
						uMode = UMode::GoToHome;
					}
				}
			}
		}
		else
		{
			if (uMode == UMode::TakeNeedItems)
			{
				auto& f = workedNode->facitity;
				for (auto& ni : f.fData->work.needItems)
				{
					if (f.numItem(ni.first) < ni.second && nowNode->facitity.numItem(ni.first) >= ni.second - f.numItem(ni.first))
					{
						nowNode->facitity.pullItem(ni.first, ni.second - f.numItem(ni.first));
						addItem(ni.first, ni.second - f.numItem(ni.first));
					}
				}

				uMode = UMode::None;
			}

			if (workedNode != nullptr)
			{
				moveTo(workedNode);
				uMode = UMode::GoToWork;
			}
		}

		if (z != 0.0) z = z + speed >= 180_deg ? 0.0 : z + speed;
	}
	else
	{
		z = z + speed >= 180_deg ? z + speed - 180_deg : z + speed;
		t += 2.0 / (nowNode->pos - route.front()->pos).length();

		if (t >= 1.0)
		{
			t -= 1.0;
			nowNode = route.front();
			route.pop_front();

			if (route.isEmpty()) t = 0.0;
		}
	}
}

Vec2	Unit::pos() const
{
	if (route.isEmpty()) return nowNode->pos + delta;
	else return nowNode->pos.lerp(route.front()->pos, t);
}

bool	Unit::mouseOver() const
{
	return circle().mouseOver();
}

Circle	Unit::circle() const
{
	auto d = pos().movedBy(0, -16.0 - abs(sin(z))*32.0);

	return Circle(d, 16.0);
}

void	Unit::draw()
{
	if (!enabled) return;

	auto d = pos().movedBy(0, -16.0 - abs(sin(z))*32.0);

	Circle(pos(), 12.0).draw(ColorF(0.0, 0.4));

	circle().draw(color);
	if(mouseOver()) circle().draw(ColorF(1.0, 0.5));
	circle().drawFrame(4.0, Palette::Black);

	for (int i = 0; i < int(items.size()); i++)
	{
		auto* item = items[i].first;

		item->texture.resize(32.0, 32.0).drawAt(d.movedBy(0, -4.0 - (8.0 + 8.0*abs(sin(z)))*(i + 1)));
	}

	String text;
	switch (uMode)
	{
	case UMode::Work:			text = L"仕事中";				break;
	case UMode::GoToWork:		text = L"仕事場に行く";			break;
	case UMode::GoToHome:		text = L"家に帰る";				break;
	case UMode::TakeNeedItems:	text = L"仕事用具を持ってくる";	break;
	case UMode::TakeRest:		text = L"休憩中";				break;
	case UMode::TakeFood:		text = L"食べ物を取りに行く";	break;
	case UMode::TakeClothes:	text = L"衣類を取りに行く";		break;
	default:
		break;
	}
	if (!text.isEmpty())
	{
		auto ff = planet->mFont(text);
		auto fr = ff.region().stretched(4.0, 0.0).movedBy(pos().movedBy(24, -64));
		RoundRect(fr, 8.0).draw(Palette::White);

		ff.drawAt(fr.center(), Palette::Black);
	}
}

void	Facility::addItem(IData* _iData, int _num)
{
	for (auto& item : items)
	{
		if (item.first == _iData)
		{
			item.second += _num;
			return;
		}
	}

	items.emplace_back(_iData, _num);
}

void	Unit::addItem(IData* _iData, int _num)
{
	for (auto& item : items)
	{
		if (item.first == _iData)
		{
			item.second += _num;
			return;
		}
	}

	items.emplace_back(_iData, _num);
}

void	Unit::moveTo(Node* _to)
{
	if (_to == nowNode) return;

	route = nowNode->getRotueTo(_to);
}

void	Unit::setWorkedNode(Node* _node)
{
	if (workedNode != nullptr) workedNode->facitity.numWork--;
	workedNode = _node;
	workedNode->facitity.numWork++;
}

Node*	Unit::searchBestLivedNode()
{
	auto* tar = workedNode != nullptr ? workedNode : nowNode;

	double min = 0.0;
	Node* result = nullptr;
	for (auto& n : planet->nodes)
	{
		auto& f = n.facitity;
		if (f.enabled && f.fData->maxLive - f.numLive > 0 && (min == 0.0 || tar->getLengthTo(&n) < min))
		{
			min = tar->getLengthTo(&n);
			result = &n;
		}
	}

	return result;
}

void	Unit::setLivedNode(Node* _node)
{
	if (livedNode != nullptr) livedNode->facitity.numLive--;
	livedNode = _node;
	livedNode->facitity.numLive++;
}

Array<Node*>	Node::getRotueTo(Node* _to)
{
	Array<Node*> nodes;

	nodes.emplace_back(_to);

	if (_to == this) return nodes;

	_to->cost = 1.0;
	_to->from = nullptr;

	for (int i = 0; i < int(nodes.size()); i++)
	{
		auto* n = nodes[i];

		for (auto* cn : n->connections)
		{
			if (cn->cost == 0 || cn->cost > n->cost + (n->pos - cn->pos).length())
			{
				nodes.emplace_back(cn);
				cn->cost = n->cost + (n->pos - cn->pos).length();
				cn->from = n;
			}
		}
	}

	for (auto* n : nodes) n->cost = 0.0;

	nodes.clear();

	for (auto* n = from; n != nullptr; n = n->from) nodes.emplace_back(n);

	return nodes;
}

double	Node::getLengthTo(Node* _to)
{
	if (this == _to) return 0.0;

	auto nodes = getRotueTo(_to);
	double length = 0.0;

	nodes.push_front(this);

	for (int i = 0; i < int(nodes.size() - 1); i++)
	{
		auto* n1 = nodes[i];
		auto* n2 = nodes[i + 1];

		length += (n1->pos - n2->pos).length();
	}

	return length;
}

int		BData::id() const
{
	return int(this - &planet->bData.front());
}

void	Planet::load(const FilePath& _path)
{
	BinaryReader bin(_path);

	//Nodeの数を取得
	{
		size_t nodeSize;
		bin.read(nodeSize);
		nodes.resize(nodeSize, this);
	}

	//Node
	for (auto& n : nodes)
	{
		//pos
		bin.read(n.pos);

		//bData
		int bDataID;
		bin.read(bDataID);
		n.bData = &bData[bDataID];

		//connections
		size_t connectionsSize;
		bin.read(connectionsSize);
		n.connections.resize(connectionsSize);
		for (int i = 0; i < int(n.connections.size()); i++)
		{
			int nodeID;
			bin.read(nodeID);
			n.connections[i] = &nodes[nodeID];
		}
	}
}

BData*	Planet::getBData(const String& _name)
{
	for (auto& bd : bData)
	{
		if (bd.name == _name) return &bd;
	}

	return nullptr;
}

IData*	Planet::getIData(const String& _name)
{
	for (auto& id : iData)
	{
		if (id.name == _name) return &id;
	}

	return nullptr;
}

bool	Node::hasFacility() const
{
	return facitity.enabled;
}

int		Node::id() const
{
	return int(this - &planet->nodes.front());
}

bool	Node::mouseOver() const
{
	return circle().mouseOver();
}

Circle	Node::circle() const
{
	return Circle(pos, bData->size);
}

Node*	Planet::mouseOveredNode()
{
	for (auto& n : nodes)
	{
		if (n.mouseOver()) return &n;
	}

	return nullptr;
}

Unit*	Planet::mouseOveredUnit()
{
	for (auto& u : units)
	{
		if (u.mouseOver()) return &u;
	}

	return nullptr;
}

Unit*	Planet::getNewUnit()
{
	for (auto& u : units)
	{
		if (!u.enabled) return &u;
	}

	return &units.emplace_back(this);
}

TinyCamera::TinyCamera(RectF _drawingRegion)
	: targetDrawingRegion(_drawingRegion)
	, smoothDrawingRegion(targetDrawingRegion)
{}

TinyCamera::TinyCamera()
	: targetDrawingRegion(Window::ClientRect())
	, smoothDrawingRegion(targetDrawingRegion)
{}

void    TinyCamera::update()
{
	//ズームイン、アウト処理。createTransformer下で動かすと崩壊するので注意。
	{
		auto tc = createTransformer();
		const double delta = 0.1*Mouse::Wheel();    //ズームによる変動
		const auto pos = targetDrawingRegion.pos + delta*(targetDrawingRegion.pos - Cursor::PosF());
		const auto size = (1.0 + delta)*targetDrawingRegion.size;
		targetDrawingRegion.set(pos, size);
	}

	//描画領域の調節
	smoothDrawingRegion.pos = smoothDrawingRegion.pos*(1.0 - followingSpeed) + targetDrawingRegion.pos*followingSpeed;
	smoothDrawingRegion.size = smoothDrawingRegion.size*(1.0 - followingSpeed) + targetDrawingRegion.size*followingSpeed;

	//視点移動処理。フルスクリーンのゲームじゃないならキーボードによる操作にした方がいいかも
	if (KeyA.pressed() || Cursor::Pos().x <= 0) targetDrawingRegion.pos.x -= targetDrawingRegion.size.y*0.01;
	if (KeyW.pressed() || Cursor::Pos().y <= 0) targetDrawingRegion.pos.y -= targetDrawingRegion.size.y*0.01;
	if (KeyD.pressed() || Cursor::Pos().x >= Window::Size().x - 1) targetDrawingRegion.pos.x += targetDrawingRegion.size.y*0.01;
	if (KeyS.pressed() || Cursor::Pos().y >= Window::Size().y - 1) targetDrawingRegion.pos.y += targetDrawingRegion.size.y*0.01;
}

Transformer2D   TinyCamera::createTransformer() const
{
	auto mat3x2 = Mat3x2::Translate(-smoothDrawingRegion.center()).scaled(Window::Size().y / smoothDrawingRegion.size.y).translated(Window::ClientRect().center());
	return Transformer2D(mat3x2, true);
}