# include <Siv3D.hpp> // OpenSiv3D v0.1.7
# include "Header.h"

void Main()
{
	Planet planet;
}

Planet::Planet()
{
	const auto nodeLineThickness = 8.0;
	const auto nodeLineColor = Palette::Red;

	pMode = PMode::None;

	//BData
	{
		JSONReader json(L"assets/biomes.json");
		for (auto j : json.arrayView()) bData.emplace_back(j, this);
	}

	nodes.reserve(4096);

	selectedBData = &bData.front();

	Window::Resize(1280, 720);
	Window::SetTitle(L"Node Maker");

	while (System::Update())
	{
		ClearPrint();
		Print(L"Node配置モード:1Key");
		Print(L"Node除去モード:2Key");
		Print(L"Node接続モード:3Key");
		Print(L"Node移動モード:4Key");
		Print(L"セーブ:S Key");
		Print(L"ロード:対象ファイルをドロップ");

		if (DragDrop::HasNewFilePaths())
		{
			auto fs = DragDrop::GetDroppedFilePaths();
			load(fs.front().path);
		}
		if (KeyS.down())
		{
			FilePath path = Format(Random(4096)) + L".bin";
			save(path);
		}

		if (Key1.down()) pMode = pMode == PMode::MakeNode ? pMode = PMode::None : PMode::MakeNode;
		if (Key2.down()) pMode = pMode == PMode::BreakNode ? pMode = PMode::None : PMode::BreakNode;
		if (Key3.down()) pMode = pMode == PMode::ConnectNode ? pMode = PMode::None : PMode::ConnectNode;
		if (Key4.down()) pMode = pMode == PMode::MoveNode ? pMode = PMode::None : PMode::MoveNode;

		tinyCamera.update();

		{
			auto tc = tinyCamera.createTransformer();

			switch (pMode)
			{
			case PMode::None:
				break;
			case PMode::MakeNode:
				Print(L"現在:Node配置モード");
				Print(selectedBData->name);

				if (KeyShift.down())
				{
					if (selectedBData == &bData.back()) selectedBData = &bData.front();
					else selectedBData++;
				}

				if (MouseL.down() && canSetNode(Cursor::PosF(), selectedBData)) setNode(Cursor::PosF(), selectedBData);

				break;
			case PMode::BreakNode:
				Print(L"現在:Node除去モード");
				if (MouseL.down() && mouseOveredNode() != nullptr) mouseOveredNode()->remove();

				break;
			case PMode::ConnectNode:
				Print(L"現在:Node接続モード");
				if (MouseL.down()) grabedNode = mouseOveredNode();
				if (MouseL.up() && grabedNode != nullptr)
				{
					auto* tar = mouseOveredNode();
					if (tar != grabedNode && tar != nullptr)
					{
						grabedNode->connections.emplace_back(tar);
						tar->connections.emplace_back(grabedNode);
					}
				}

				break;
			case PMode::MoveNode:
				Print(L"現在:Node移動モード");
				if (MouseL.down()) grabedNode = mouseOveredNode();
				if (MouseL.up() && grabedNode != nullptr)
				{
					bool flag = true;

					for (auto& n : nodes)
					{
						if (&n != grabedNode && n.enabled && n.circle().intersects(Circle(Cursor::PosF(), grabedNode->bData->size))) flag = false;
					}

					if (flag) grabedNode->pos = Cursor::PosF();
				}

				break;
			default:
				break;
			}


			//Node接続の描画
			for (auto& n : nodes)
			{
				if (!n.enabled) continue;

				for (auto* cn : n.connections)
				{
					Line(n.pos, cn->pos).draw(nodeLineThickness, nodeLineColor);
				}
			}

			//接続予定ライン
			if (pMode == PMode::ConnectNode && MouseL.pressed() && grabedNode != nullptr)
			{
				auto* mon = mouseOveredNode();
				if (mon != nullptr && mon != grabedNode)
				{
					Line(mon->pos, grabedNode->pos).draw(nodeLineThickness, ColorF(nodeLineColor, 0.6));
				}
				else
				{
					Line(Cursor::PosF(), grabedNode->pos).draw(nodeLineThickness, ColorF(nodeLineColor, 0.6));
				}
			}

			//Node描画
			for (auto& n : nodes)
			{
				if (!n.enabled) continue;

				n.circle().draw(n.bData->color);
				if (n.circle().mouseOver()) n.circle().drawFrame(4.0, ColorF(Palette::Red, 0.5));
			}

			//配置予定のNode
			if (pMode == PMode::MakeNode)
			{
				if (canSetNode(Cursor::PosF(), selectedBData)) Circle(Cursor::PosF(), selectedBData->size).draw(ColorF(selectedBData->color, 0.6));
				else Circle(Cursor::PosF(), selectedBData->size).draw(ColorF(Palette::Red, 0.6));
			}

			//移動予定のNode
			if (pMode == PMode::MoveNode && grabedNode != nullptr)
			{
				bool flag = true;

				for (auto& n : nodes)
				{
					if (&n != grabedNode && n.enabled && n.circle().intersects(Circle(Cursor::PosF(), grabedNode->bData->size))) flag = false;
				}

				if (flag) Circle(Cursor::PosF(), grabedNode->bData->size).draw(ColorF(selectedBData->color, 0.6));
				else Circle(Cursor::PosF(), grabedNode->bData->size).draw(ColorF(Palette::Red, 0.6));
			}
		}

		if (!MouseL.pressed()) grabedNode = nullptr;
	}
}

int		BData::id() const
{
	return int(this - &planet->bData.front());
}

void	Planet::save(const FilePath& _path)
{
	BinaryWriter bin(_path);

	{
		size_t nodesSize = 0;

		for (auto& n : nodes)
		{
			if (n.enabled) nodesSize++;
		}

		bin.write(nodesSize);
	}
	
	for (auto& n : nodes)
	{
		if (!n.enabled) continue;

		bin.write(n.pos);
		bin.write(n.bData->id());
		bin.write(n.connections.size());
		for (auto* cn : n.connections) bin.write(cn->id());
	}

	bin.close();
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
		//enabled
		n.enabled = true;

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

int		Node::id() const
{
	int result = 0;
	for (auto& n : planet->nodes)
	{
		if (&n == this) return result;
		if (n.enabled) result++;
	}
	return -1;
	return int(this - &planet->nodes.front());
}

Circle	Node::circle() const
{
	return Circle(pos, bData->size);
}

void	Node::remove()
{
	enabled = false;

	for (auto& cn : connections) cn->connections.remove(this);

	connections.clear();
}

bool	Planet::canSetNode(const Vec2& _pos, BData* _bData) const
{
	auto c = Circle(_pos, _bData->size);

	for (auto& n : nodes)
	{
		if (n.enabled && n.circle().intersects(c)) return false;
	}

	return true;
}

void	Planet::setNode(const Vec2& _pos, BData* _bData)
{
	auto* n = newNode();
	n->enabled = true;
	n->pos = _pos;
	n->bData = _bData;
}

Node*	Planet::newNode()
{
	for (auto& n : nodes)
	{
		if (!n.enabled) return &n;
	}

	return &nodes.emplace_back(this);
}

Node*	Planet::mouseOveredNode()
{
	for (auto& n : nodes)
	{
		if (n.enabled && Circle(n.pos, n.bData->size).mouseOver()) return &n;
	}

	return nullptr;
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
	if (Cursor::Pos().x <= 0) targetDrawingRegion.pos.x -= targetDrawingRegion.size.y*0.01;
	if (Cursor::Pos().y <= 0) targetDrawingRegion.pos.y -= targetDrawingRegion.size.y*0.01;
	if (Cursor::Pos().x >= Window::Size().x - 1) targetDrawingRegion.pos.x += targetDrawingRegion.size.y*0.01;
	if (Cursor::Pos().y >= Window::Size().y - 1) targetDrawingRegion.pos.y += targetDrawingRegion.size.y*0.01;
}

Transformer2D   TinyCamera::createTransformer() const
{
	auto mat3x2 = Mat3x2::Translate(-smoothDrawingRegion.center()).scaled(Window::Size().y / smoothDrawingRegion.size.y).translated(Window::ClientRect().center());
	return Transformer2D(mat3x2, true);
}