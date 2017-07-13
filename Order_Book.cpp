/*
 Program Name: Order Book
 Author: Kunal Zantye
 Program Intent: Find Match between opposite transaction
 Input Data: STDIN and STDOUT
 Output Data: Match results
 Compile Note: Use g++ to compile.
 
README

Few Assumptions
- Market orders are executed till opposing orders are present or else the order is dropped.
- Sell side Stop orders are triggered when taker is sell side and the price is less than or equal to given threshold.
- Buy side Stop orders are triggered when taker is buy side and the price is less than or equal to given threshold.
 */

/*---------------
 Include Section
 ----------------*/
#include <map>
#include <string>
#include <limits>
#include <climits>
#include <vector>
#include <fstream>
#include <numeric>
#include <iostream>
#include <algorithm>
#include <iomanip> 


using namespace std;

class OrderBook
{
private:
	static bool instanceFlag;
    static OrderBook *instance;
    static int orderID;
	
	OrderBook(){} //private Constructor
	OrderBook(OrderBook const& copy);            // Not Implemented
    OrderBook& operator=(OrderBook const& copy); // Not Implemented

	typedef struct OrderData{

		string type, side;
		int ID;
		long volume;
		double price;
		OrderData(string type, string side, long volume, double price):
		 ID(orderID++), type(type), side(side), volume(volume), price(price){}
	}OrderData;
	
	typedef struct MatchData
	{
		int taker, maker;
		long volume;
		double price;
		MatchData(int taker, int maker, long volume, double price): 
		taker(taker), maker(maker), volume(volume), price(price){}
	}MatchData;
	vector<MatchData> match;
	map<int, pair<long, double>> buyItems, sellItems, stopBuyItems, stopSellItems;
public:

	static OrderBook* getInstance();
	~OrderBook(){
		instanceFlag=false;
	}

	// Class member functions
	void ExecuteTrade(string type, string side,long volume, double price);
	void ShowOrders();
	void RemoveTransaction(int ID);
	int GetMaxBuyBid();
	int GetMinSellBid();
	int GetMinBuyThreshold(double price);
	int GetMaxSellThreshold(double price);
	void ExecuteStop(int ID, bool buySellFlag);
	void FindMatchSell(OrderData o, bool marketFlag);
	void FindMatchBuy(OrderData o, bool marketFlag);
	void Execute(OrderData order);
	
};

bool OrderBook::instanceFlag = false;
OrderBook* OrderBook::instance = NULL;
int OrderBook:: orderID=1;

// Returns the single instance of the class
OrderBook* OrderBook::getInstance()
{
    if(! instanceFlag)
    {
        instance = new OrderBook();
        instanceFlag = true;
        return instance;
    }
    else
    {
        return instance;
    }
}

// Intialize struct OrderData with input data
// Execute the current order
void OrderBook :: ExecuteTrade(string type, string side,long volume, double price)
{
	OrderData order(type,side,volume,price);
	OrderBook :: Execute(order);
}

// Display all thr matches found in the given transaction
// Display the vector of struct MatchData
void OrderBook :: ShowOrders()
{
	cout<<"\nbuyItems Map\n";
	for(auto m: buyItems)
	{
		cout<<m.first<<" "<<m.second.first<<" "<<m.second.second<<endl;
	}
	cout<<"\nsellItems Map\n";
	for(auto m: sellItems)
	{
		cout<<m.first<<" "<<m.second.first<<" "<<m.second.second<<endl;
	}
	cout<<"\nStop buyItems Map\n";
	for(auto m: stopBuyItems)
	{
		cout<<m.first<<" "<<m.second.first<<" "<<m.second.second<<endl;
	}
	cout<<"\nStop sellItems Map\n";
	for(auto m: stopSellItems)
	{
		cout<<m.first<<" "<<m.second.first<<" "<<m.second.second<<endl;
	}
	cout<<"\nMatch vector\n";
	for(auto e: match)
		cout<<"match "<<e.taker<<" "<<e.maker<<" "<<e.volume<<" "<<setprecision (2) << fixed <<e.price<<endl;

}

// Remove entries from buyItems Map or sellItems map
void OrderBook :: RemoveTransaction(int ID)
{

	// Remove from buy map
	auto buyItemsItr=buyItems.find(ID);
	if(buyItemsItr!=buyItems.end())
		buyItems.erase(buyItemsItr);
	// Remove from sell map
	auto sellItemsItr=sellItems.find(ID);
	if(sellItemsItr!=sellItems.end())
		sellItems.erase(sellItemsItr);
}

// Return the oldest max buying price ID
int OrderBook :: GetMaxBuyBid(){
	vector<int> maxBid;
	int maxPrice=0;
	// Add all max buying price in vector
	for(auto element: buyItems)
	{
		if(element.second.second > maxPrice)
		{
			maxPrice=element.second.second;
			maxBid.clear();
			maxBid.push_back(element.first);
		}
		else if(element.second.second == maxPrice)
		{
			maxBid.push_back(element.first);	
		}
	}
	// Return the oldest max buying price ID
	int max=maxBid[0];
	maxBid.erase(maxBid.begin());
	return max;

}

// Return the oldest min selling price ID
int OrderBook :: GetMinSellBid(){
vector<int> minBid;
	int minPrice=INT_MAX;
	// Add all min selling price in vector
	for(auto element: sellItems)
	{
		if(element.second.second < minPrice)
		{
			minPrice=element.second.second;
			minBid.clear();
			minBid.push_back(element.first);
		}
		else if(element.second.second == minPrice)
		{
			minBid.push_back(element.first);	
		}
	}
	// Return the oldest min selling price ID
	int min=minBid[0];
	minBid.erase(minBid.begin());
	return min;
}



int OrderBook :: GetMinBuyThreshold(double price){
	int thresholdID;
	for (auto element: stopBuyItems)
	{
		if(price >= element.second.second)
			return element.first;
	}
	return -1;
}

int OrderBook :: GetMaxSellThreshold(double price){
	int thresholdID;
	for (auto element: stopSellItems)
	{
		if(price <= element.second.second)
			return element.first;

	}
	return -1;

}

void OrderBook :: ExecuteStop(int ID, bool buySellFlag)
{
	// Find match for sell
	if(buySellFlag)
	{
		if(stopBuyItems.find(ID)!=stopBuyItems.end())
		{
			auto stopVolumePrice=stopBuyItems[ID];
			while(stopVolumePrice.first!=0)
			{
				if(sellItems.empty() ){
					if(stopBuyItems.find(ID)!=stopBuyItems.end())
						stopBuyItems.erase(stopBuyItems.find(ID));
					break;
				}
				else{
					// Find min sell bid
					long tradedVolume;
					int minBidID=GetMinSellBid();
					auto volumePrice=sellItems[minBidID];
					// Update sellItems map volume if buy volume is less than sell volume
					if(stopVolumePrice.first < volumePrice.first)
					{

						volumePrice.first-=stopVolumePrice.first;
						sellItems[minBidID]=volumePrice;
						tradedVolume=stopVolumePrice.first;
						stopVolumePrice.first=0;
						if(stopBuyItems.find(ID)!=stopBuyItems.end())
							stopBuyItems.erase(stopBuyItems.find(ID));
					}
					else{
						// Or else delete map value
						// Update buy volume
						// Update volume in stopBuyItems Map
						stopVolumePrice.first-=volumePrice.first;
						stopBuyItems[ID]=stopVolumePrice;
						tradedVolume=volumePrice.first;
						RemoveTransaction(minBidID);
					}
					//insert in match
					match.push_back(MatchData(ID,minBidID,tradedVolume,volumePrice.second));
				}
			}// end while
		} // end outside while if
	}
	// Find match for buy
	else
	{
		if(stopSellItems.find(ID)!=stopSellItems.end())
		{
			auto stopVolumePrice=stopSellItems[ID];
			while(stopVolumePrice.first!=0){
				if(buyItems.empty() ){
					if(stopSellItems.find(ID)!=stopSellItems.end())
						stopSellItems.erase(stopSellItems.find(ID));
					break;
				}
				else{
					// Find max buy bid
					long tradedVolume;
					int maxBidID=GetMaxBuyBid();
					auto volumePrice= buyItems[maxBidID];
					// Update buyItems map volume if sell volume is less than buy volume
					if(stopVolumePrice.first < volumePrice.first)
					{
						volumePrice.first-=stopVolumePrice.first;
						buyItems[maxBidID]=volumePrice;
						tradedVolume=stopVolumePrice.first;
						stopVolumePrice.first=0;
						if(stopSellItems.find(ID)!=stopSellItems.end())
							stopSellItems.erase(stopSellItems.find(ID));
					}
					else
					{
						// Or else delete map value
						// Update Sell volume 
						// Update volume in stopSellItems Map
						stopVolumePrice.first-=volumePrice.first;
						stopSellItems[ID]=stopVolumePrice;
						tradedVolume=volumePrice.first;
						RemoveTransaction(maxBidID);
					}
					//insert in match vector
					match.push_back(MatchData(ID,maxBidID,tradedVolume,volumePrice.second));


				}

			}// end while
		}// end outside while if
	}// end else
}

// For buy side find match for sell side
// marketflag is true when order transaction is market
void OrderBook :: FindMatchSell(OrderData o, bool marketFlag){
	// Check if opposite side present
	// Or add to buyItems map
	if(sellItems.empty() && marketFlag == false)
		buyItems[o.ID]=make_pair(o.volume, o.price);
	else
	{
		while(o.volume != 0 ){
			

			if(sellItems.empty())
			{
				if(marketFlag == false)
					buyItems[o.ID]=make_pair(o.volume, o.price);	
				break;
			}
			else
			{
				// Find min sell bid
				long tradedVolume;
				int minBidID=GetMinSellBid();
				auto volumePrice=sellItems[minBidID];
				
				if(o.price < volumePrice.second && o.price !=0 && marketFlag==false)
				{
					if(marketFlag == false)
						buyItems[o.ID]=make_pair(o.volume, o.price);	
					break;
				
				}
				else{
					// Update sellItems map volume if buy volume is less than sell volume
					if(o.volume < volumePrice.first)
					{

						volumePrice.first-=o.volume;
						sellItems[minBidID]=volumePrice;
						tradedVolume=o.volume;
						o.volume=0;
					}
					else{
						// Or else delete map value
						// Update buy volume 
						o.volume-=volumePrice.first;
						tradedVolume=volumePrice.first;
						RemoveTransaction(minBidID);
					}
					match.push_back(MatchData(o.ID,minBidID,tradedVolume,volumePrice.second));

					if(!stopBuyItems.empty())
					{
						int stopBuyID=GetMinBuyThreshold(volumePrice.second);
						//auto stopVolumePrice= stopBuyItems[stopBuyID];
						// If buy threshold matched 
						// Call ExecuteStop with buySellFlag as true
					 	if(stopBuyID!=-1 )
					 	{
					 		ExecuteStop(stopBuyID, true);
					 	}
					}
				}
			} // end inside while else
		}// end while loop
	}// end outside while else
}

// For sell side find match for buy side
// marketflag is true when order transaction is market
void OrderBook :: FindMatchBuy(OrderData o, bool marketFlag){
	// Check if opposite side present
	// Or add to sellItems map
	if(buyItems.empty() && marketFlag == false)
		sellItems[o.ID]=make_pair(o.volume, o.price);
	else
	{
		while(o.volume != 0 ){

			if(buyItems.empty())
			{
				if(marketFlag == false)
					sellItems[o.ID]=make_pair(o.volume, o.price);	
				break;
			}
			else
			{
				// Find max buy bid
				long tradedVolume;
				int maxBidID=GetMaxBuyBid();
				auto volumePrice= buyItems[maxBidID];
				if(o.price > volumePrice.second && o.price !=0 && marketFlag==false)
				{
					if(marketFlag == false)
						sellItems[o.ID]=make_pair(o.volume, o.price);	
					break;
				
				}
				else{
					// Update buyItems map volume if sell volume is less than buy volume
					if(o.volume < volumePrice.first)
					{
						volumePrice.first-=o.volume;
						buyItems[maxBidID]=volumePrice;
						tradedVolume=o.volume;
						o.volume=0;
					}
					else
					{
						// Or else delete map value
						// Update Sell volume 
						o.volume-=volumePrice.first;
						tradedVolume=volumePrice.first;
						RemoveTransaction(maxBidID);
					}
					// Insert into match
					match.push_back(MatchData(o.ID,maxBidID,tradedVolume,volumePrice.second));

					//check for stop condition
					if(!stopSellItems.empty())
					{
						int stopSellID=GetMaxSellThreshold(volumePrice.second);
						
						// If sell threshold matched 
						// Call ExecuteStop with buySellFlag as false
					 	if(stopSellID!=-1)
					 	{
					 		ExecuteStop(stopSellID, false);
					 	}
					}

				}
				
			}// end inside while else

		}// end while loop
	}// end outside while else

}

// Parse the order request
// Check for different transaction
// Market, limit, stop, cancel
void OrderBook :: Execute(OrderData order)
{
		if(order.type=="market")
		{
			if(order.side=="buy")
				FindMatchSell(order, true);
			else if(order.side=="sell")
				FindMatchBuy(order, true);
		}
		if(order.type== "limit")
		{
			if(order.side=="buy")
				FindMatchSell(order, false);
			else if(order.side=="sell")
				FindMatchBuy(order, false);
		}

		if(order.type=="stop")
		{
			if(order.side=="buy")
				stopBuyItems[order.ID]=make_pair(order.volume, order.price);
			else if(order.side=="sell")
				stopSellItems[order.ID]=make_pair(order.volume, order.price);
			
		}
		if(order.type=="cancel")
		{	
			RemoveTransaction(order.volume);
			if(stopBuyItems.find(order.volume)!=stopBuyItems.end())
				stopBuyItems.erase(stopBuyItems.find(order.volume));
			if(stopSellItems.find(ID)!=stopSellItems.end())
						stopSellItems.erase(stopSellItems.find(ID));		
		}	
}

int main() {
   
    OrderBook *o;
    o=OrderBook::getInstance();

    ifstream file("input.txt");
	

	while(!file.eof())
	{
	    string type;
	    string side;
	    string volume;
	    string price;

	   file >> type >> side >> volume >> price;

	    o->ExecuteTrade(type, side, stoi(volume), stof(price));
        
	}
	//o->ExecuteTrade();
    o->ShowOrders();
    return 0;
}