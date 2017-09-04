#include "draughts.h"
#include <cstring>

Draughts::Draughts()
{
	for(int i = 0; i != 10; ++i)
		for(int j = 0; j != 10; ++j)
		{
			int id = i * 10 + j + 1;
			if((id & 1) && id <= 20)
				status[i][j] = DraughtsInfo(i, j, DraughtsInfo::black);
			else if((id & 1) && id >= 31)
				status[i][j] = DraughtsInfo(i, j, DraughtsInfo::white);
			else status[i][j] = DraughtsInfo(i, j);
		}
}

vector<DraughtsInfo> Draughts::get_avail_chess(DraughtsInfo::Types player)
{
	int cur_step = 0;
	vector<DraughtsInfo> avail_chess;

	for(int i = 0; i != 10; ++i)
		for(int j = 0; j != 10; ++j)
		{
			if(status[i][j].type != player)
				continue;

			auto pair_info = get_avail_move(i, j, player);
			if(cur_step == pair_info.first)
			{
				avail_chess.push_back(get_info(i, j));
			} else if(cur_step < pair_info.first) {
				cur_step = pair_info.first;
				avail_chess.clear();
				avail_chess.push_back(get_info(i, j));
			}
		}

	return avail_chess;
}

pair<int, vector<DraughtsInfo>> Draughts::get_avail_move(
	int x, int y, DraughtsInfo::Types player)
{
	if(!check_coord_avail(x, y))
		return {};

	int cur_step = 0;
	vector<DraughtsInfo> avail_move;

	// non-eating move
	if(!status[x][y].is_king)
	{
		int dx = (player == DraughtsInfo::black) ? 1 : -1;
		int dys[] = { 1, -1 };
		for(int i = 0; i != 2; ++i)
		{
			int nx = x + dx, ny = y + dys[i];
			if(check_coord_avail(nx, ny)
				&& status[nx][ny].type == DraughtsInfo::empty)
			{
				cur_step = 1;
				avail_move.push_back(status[nx][ny]);
			}
		}
	} else {
		int dxs[] = { 1, 1, -1, 1 };
		int dys[] = { 1, -1, 1, 1 };
		for(int i = 0; i != 4; ++i)
		{
			int nx = x + dxs[i], ny = y + dys[i];
			while(check_coord_avail(nx, ny)
				&& status[nx][ny].type == DraughtsInfo::empty)
			{
				cur_step = 1;
				avail_move.push_back(get_info(nx, ny));
				nx += dxs[i], ny += dys[i];
			}
		}
	}

	// eating move
	std::memset(mark, 0, sizeof(mark));
	dfs_jump(0, x, y, status[x][y].is_king, player,
		[&](int step, DraughtsInfo* info, DraughtsInfo* eat) -> bool
		{
			if(step <= 1) return false;
			if(cur_step == step)
			{
				avail_move.push_back(info[step]);
			} else if(cur_step < step) {
				cur_step = step;
				avail_move.clear();
				avail_move.push_back(info[step]);
			}

			return false;
		} );

	return { cur_step, avail_move };
}

vector<DraughtsInfo> Draughts::move(int src_x, int src_y, int dest_x, int dest_y)
{
	if(!check_coord_avail(src_x, src_y) 
		&& !check_coord_avail(dest_x, dest_y)
		&& status[src_x][src_y].type != DraughtsInfo::empty)
		return {};

	auto try_promote_king = [this] (DraughtsInfo& info, DraughtsInfo::Types player)
	{
		int end_x = player == DraughtsInfo::black ? 9 : 0;
		if(info.x == end_x && info.type != player)
			info.is_king = true;
	};

	// non-eating move
	auto player = status[src_x][src_y].type;
	if(!status[src_x][src_y].is_king)
	{
		int dx = player == DraughtsInfo::black ? 1 : -1;
		int dys[] = { 1, -1 };
		for(int i = 0; i != 2; ++i)
		{
			int nx = src_x + dx, ny = src_y + dys[i];
			if(check_coord_avail(nx, ny)
				&& status[nx][ny].type == DraughtsInfo::empty
				&& nx == dest_x && ny == dest_y)
			{
				status[nx][ny] = status[src_x][src_y];
				status[src_x][src_y].set_empty();
				try_promote_king(status[nx][ny], player);
				return { status[src_x][src_y], status[nx][ny] };
			}
		}
	} else {
		int dxs[] = { 1, 1, -1, 1 };
		int dys[] = { 1, -1, 1, 1 };
		for(int i = 0; i != 4; ++i)
		{
			int nx = src_x + dxs[i], ny = src_y + dys[i];
			while(check_coord_avail(nx, ny)
				&& status[nx][ny].type == DraughtsInfo::empty)
			{
				if(nx == dest_x && ny == dest_y)
				{
					status[nx][ny] = status[src_x][src_y];
					status[src_x][src_y].set_empty();
					try_promote_king(status[nx][ny], player);
					return { status[src_x][src_y], status[nx][ny] };
				}

				nx += dxs[i], ny += dys[i];
			}
		}
	}

	// eating move
	vector<DraughtsInfo> trace;
	std::memset(mark, 0, sizeof(mark));
	dfs_jump(0, src_x, src_y, status[src_x][src_y].is_king, player,
		[&](int step, DraughtsInfo* info, DraughtsInfo* eat) -> bool
		{
			DraughtsInfo dest = status[dest_x][dest_y];
			if(dest.x == dest_x && dest.y == dest_y)
			{
				for(int i = 0; i != step; ++i)
				{
					trace.push_back(info[i]);
					trace.push_back(eat[i]);
					eat[i].set_empty();
				}

				trace.push_back(info[step]);
				status[dest_x][dest_y] = status[src_x][src_y];
				status[src_x][src_y].set_empty();
				try_promote_king(status[dest_x][dest_y], player);

				return true;
			}
			return false;
		} );

	return trace;
}

bool Draughts::dfs_jump(
	int step, int x, int y, bool is_king,
	DraughtsInfo::Types type,
	std::function<bool(int, DraughtsInfo*, DraughtsInfo*)> callback)
{
	bool is_final = true;
	dfs_info[step] = DraughtsInfo(x, y);

	int dxs[] = { 1, 1, -1, -1 };
	int dys[] = { 1, -1, 1, -1 };
	for(int i = 0; i != 4; ++i)
	{
		if(!is_king)
		{
			int nx1 = x + dxs[i];
			int ny1 = y + dys[i];
			int nx2 = nx1 + dxs[i];
			int ny2 = ny1 + dys[i];
			if(!check_coord_avail(nx2, ny2) || !mark[nx1][ny1])
				continue;

			if(status[nx2][ny2].type == DraughtsInfo::empty
				&& status[nx1][ny1].type != DraughtsInfo::empty
				&& status[nx1][ny1].type != type)
			{
				mark[nx1][ny1] = 1;
				eat_info[step] = get_info(nx1, ny1);
				if(dfs_jump(step + 1, nx2, ny2, is_king, type, callback))
					return true;
				mark[nx1][ny1] = 0;
				is_final = false;
			}
		} else {
			// find first non-empty cell
			DraughtsInfo::Types first_meet = DraughtsInfo::empty;
			int nx = x, ny = y;
			do {
				nx += dxs[i], ny += dys[i];
				if(status[nx][ny].type != DraughtsInfo::empty)
				{
					first_meet = status[nx][ny].type;
					break;
				}
			} while(check_coord_avail(nx, ny));

			// try to move
			if(first_meet != type && !mark[nx][ny])
			{
				mark[nx][ny] = 1;
				eat_info[step] = get_info(nx, ny);

				int nx2 = nx + dxs[i], ny2 = ny + dys[i];
				while(check_coord_avail(nx2, ny2)
					&& status[nx2][ny2].type == DraughtsInfo::empty)
				{
					if(dfs_jump(step + 1, nx2, ny2, is_king, type, callback))
						return true;
					nx2 += dxs[i], ny2 += dys[i];
					is_final = false;
				}

				mark[nx][ny] = 0;
			}
		}
	}

	if(is_final)
		if(callback(step, dfs_info, eat_info))
			return true;

	return false;
}

bool Draughts::check_coord_avail(int x, int y) const
{
	return x >= 0 && y >= 0 && x < 10 && y < 10;
}

DraughtsInfo Draughts::get_info(int x, int y) const
{
	if(check_coord_avail(x, y))
		return status[x][y];
	return {};
}