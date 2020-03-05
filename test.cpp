

#include <vector>
#include <map>
#include <utility>
#include <string>
#include <cstdio>
#include <iostream>
#include <set>

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/encodings.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/encodedstream.h"

#include "util.h"


#include "murmur3.h"

//#include <boost/algorithm/string/replace.hpp>
//#include <boost/regex.hpp>
#include <regex>
#include <math.h>
#include <stdarg.h>

struct Bits128 {
  uint64_t a,b;
  bool operator<(const Bits128 &o) const {

    if (o.a == a) {
      return o.b < b;
    } else {
      return o.a < a;
    }
  }
};

std::string string_format(const std::string fmt, ...) {
    int size = 100;
    std::string str;
    va_list ap;
    while (1) {
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *) str.c_str(), size, fmt.c_str(), ap);
        va_end(ap);
        if (n > -1 && n < size) {
            str.resize(n);
            return str;
        }
        if (n > -1)
            size = n + 1;
        else
            size *= 2;
    }
    return str;
}
struct Color {
  double r,g,b;
};
Color hsv2rgb(double h, double s, double v) {
    double i;
    Color c;
    double f, p, q, t;
    while (h > 1)
        h -= 1.0;
    while (h < 0)
        h += 1.0;

    if (s == 0) {
        c.r = v;
        c.g = v;
        c.b = v;
    } else {
        h *= 6.0;
        i = floor(h);
        f = h - (double) i;
        p = v * (1 - s);
        q = v * (1 - s * f);
        t = v * (1 - s * (1 - f));
        switch ((int) i) {
            case 0:
                c.r = v;
                c.g = t;
                c.b = p;
                break;
            case 1:
                c.r = q;
                c.g = v;
                c.b = p;
                break;
            case 2:
                c.r = p;
                c.g = v;
                c.b = t;
                break;
            case 3:
                c.r = p;
                c.g = q;
                c.b = v;
                break;
            case 4:
                c.r = t;
                c.g = p;
                c.b = v;
                break;
            default:
                c.r = v;
                c.g = p;
                c.b = q;
                break;
        }
    }
    return c;
};
template<typename Ch, typename Tr>
std::basic_ostream<Ch, Tr> &operator<<(std::basic_ostream<Ch, Tr> &ostream, const Color &c) {
    ostream << string_format("#%02x%02x%02x", (uint8_t) (c.r * 255.0),(uint8_t) (c.g * 255.0),(uint8_t) (c.b * 255.0)  );
    return ostream;
}

struct World {
  typedef std::size_t resourceindex;
  typedef std::size_t recipeindex;

  struct Recipe {
    std::string name;
    recipeindex index;
    std::map<resourceindex,double> input, output, neteffect;

    void calculateNet(){
      Recipe* this2 = this;
      sortedPairContainerComparison(input,
				    output,
				    [this2](const std::map<const resourceindex,double>::const_iterator& input_only)->bool{
				      this2->neteffect[input_only->first] = -input_only->second;
				      return true;
				    },
				    [this2](const std::map<const resourceindex,double>::const_iterator& output_only)->bool{
				      this2->neteffect[output_only->first] = output_only->second;
				      return true;
				    },
				    [this2](const std::map<const resourceindex,double>::const_iterator& input,
					    const std::map<const resourceindex,double>::const_iterator& output)->bool{
				      this2->neteffect[input->first] = output->second - input->second;
				      return true;
				    });
    }
  };

  struct Resource {
    std::string name;
    resourceindex index;
    bool isliquid;
    double energyMJ;
    bool isresource; // ores, oil, water are resources
    std::set<recipeindex> allprecursors, producedby, usedby; // not the resources but the recipes that directly or indirectly are involved with creating this resource.

  };

  std::map<std::string,resourceindex> resourcename_to_resourceindex;
  std::vector<Resource> resources;

  std::map<std::string,recipeindex> recipe_to_recipeindex;
  std::vector<Recipe> recipes;

  std::map<std::string,double> powerconsumptionmw; // for the buildings...

  resourceindex getResource(const std::string& n){
    auto i = resourcename_to_resourceindex.find(n);
    if (i != resourcename_to_resourceindex.end()){
      return i->second;
    }
    resourcename_to_resourceindex.insert(std::make_pair(n, resources.size()));
    resources.push_back(Resource{n, resources.size(),false,0,false});
    return resources.size() - 1;
  }
  resourceindex getResourceNC(const std::string& n) const{
    auto i = resourcename_to_resourceindex.find(n);
    assertss (i != resourcename_to_resourceindex.end(), n);
      return i->second;
  }

  recipeindex getRecipe(const std::string& n){
    const auto i = recipe_to_recipeindex.find(n);
    if (i != recipe_to_recipeindex.end()){
      return i->second;
    }
    recipe_to_recipeindex.insert(std::make_pair(n, recipes.size()));
    recipes.push_back(Recipe{n, recipes.size()});
    return recipes.size() - 1;
  }
  recipeindex getRecipeNC(const std::string& n) const {
    const auto i = recipe_to_recipeindex.find(n);
    assertss(i != recipe_to_recipeindex.end(),n);
    return i->second;
  }

  recipeindex concatenateRecipes(const recipeindex recipea, const recipeindex recipeb, const resourceindex transferred_item, const std::string newrecipename) {
    Recipe& ret = recipes[getRecipe(newrecipename)];
    Recipe& r1 = recipes[recipea];
    Recipe& r2 = recipes[recipeb];
    double produced_by_r1 = 0;
    double used_by_r2 = 0;
    {
      auto i = r1.output.find(transferred_item);
      assert (i != r1.output.end());
      produced_by_r1 = i->second;
    }
    {
      auto i = r2.input.find(transferred_item);
      assert (i != r2.input.end());
      used_by_r2 = i->second;
    }
    double scale_factor = used_by_r2 / produced_by_r1;
    for (auto& i : r1.input){
      ret.input[i.first] += i.second * scale_factor;
    }
    for (auto& i : r1.output){
      if (i.first != transferred_item){
	ret.output[i.first] += i.second * scale_factor;
      }
    }
    for (auto& i : r2.input){
      if (i.first != transferred_item){
	ret.input[i.first] += i.second;
      }
    }
    for (auto& i : r2.output){
      ret.output[i.first] += i.second;
    }
    ret.calculateNet();
    return ret.index;
  }

  void calculatePrecursors(){ // this seems kinda useless actually ...
    // pfrt should i invest time in doing this efficiently??

    std::set<recipeindex> todoes;
    for (recipeindex i = 0; i < recipes.size(); ++i){
      todoes.insert(i);
    }
    while (!todoes.empty()){
      const Recipe& currentrecipe = recipes[*todoes.begin()];
      todoes.erase(todoes.begin());
      for (const auto& i : currentrecipe.neteffect){
	if (i.second > 0){
	  auto& outputresource = resources[i.first];
	  auto& precursors = outputresource.allprecursors;
	  const std::size_t before = precursors.size();
	  precursors.insert(currentrecipe.index);
	  for (const auto& in: currentrecipe.input){
	    precursors.insert(resources[in.first].allprecursors.begin(),resources[in.first].allprecursors.end());
	  }
	  if (precursors.size() != before){
	    todoes.insert(outputresource.usedby.begin(),outputresource.usedby.end());
	  }
	}
      }
    }

    /*    for (auto& resource : resources) {
      std::cerr << resource.name << " has " << resource.allprecursors.size() << " precursor-recipes." << std::endl;
      } */

  }

  void print() {
    for (const auto& recipe : recipes){
      std::cerr << recipe.name << std::endl;
      for (const auto& i : recipe.input ){
	std::cerr << "  in: " << resources[i.first].name << " X " << i.second << std::endl ;
      }
      for (const auto& i : recipe.output){
	std::cerr << " out: " << resources[i.first].name << " X " << i.second << std::endl ;
      }
      for (const auto& i : recipe.neteffect){
	std::cerr << " effect: " << resources[i.first].name << " X " << i.second << std::endl ;
      }
    }
  }

  void loadFromDerivedJson(const std::string& jsonfilename){
    FILE* fp = fopen(jsonfilename.c_str(), "rb");
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document jsonrecipes;
    jsonrecipes.ParseStream(is);
    assert(jsonrecipes.IsArray());

    for (const auto& jsonrecipe : jsonrecipes.GetArray()){
      const double durationminutes = std::stod(jsonrecipe["mManufactoringDuration"].GetString()) / 60.0;
      Recipe& recipe = recipes[getRecipe(jsonrecipe["mDisplayName"].GetString())];
      for (const auto& jsonrecipeinput : jsonrecipe["mIngredients"].GetArray()  ) {
	Resource& input = resources[getResource(jsonrecipeinput["ItemClass"].GetString())];
	recipe.input[input.index] = std::stod(jsonrecipeinput["Amount"].GetString()) / durationminutes;
	input.usedby.insert(recipe.index);
      }
      for (const auto& jsonrecipeoutput : jsonrecipe["mProduct"].GetArray()  ) {
	Resource& output = resources[getResource(jsonrecipeoutput["ItemClass"].GetString())];
	recipe.output[output.index] = std::stod(jsonrecipeoutput["Amount"].GetString()) / durationminutes;
	output.producedby.insert(recipe.index);
      }

      std::string building = jsonrecipe["mProducedIn"].GetString();
      if (building == "AssemblerMk1" ){
        recipe.input[getResource("energyMJ")] = durationminutes * 16.0 ; //TODO: check this...
      }else if (building == "ConstructorMk1" ){
        recipe.input[getResource("energyMJ")] = durationminutes * 4.0 ;
      }else if (building == "FoundryMk1" ){
        recipe.input[getResource("energyMJ")] = durationminutes * 16.0 ;
      }else if (building == "ManufacturerMk1" ){
        recipe.input[getResource("energyMJ")] = durationminutes * 50.0 ;
      }else if (building == "OilRefinery" ){
        recipe.input[getResource("energyMJ")] = durationminutes * 30.0 ;
      }else if (building == "SmelterMk1" ){
        recipe.input[getResource("energyMJ")] = durationminutes * 4.0 ;
      }else if (building == "AutomatedWorkBench" ){
//        recipe.input[getResource("energyMJ")] = energy * 4.0 ;
      }else {
 assertss(false, "unknown factory: " << building);
      }

      recipe.calculateNet();
    }

    {
      Recipe& recipe = recipes[getRecipe("turbofuelenergy")]; // 22GW = 300k oil
      recipe.input[getResourceNC("LiquidTurboFuel")] = 3;
      recipe.output[getResource("energyMJ")] = 220;
      recipe.calculateNet();
    }
    {
      Recipe& recipe = recipes[getRecipe("fuelenergy")]; // 8GW = 300k fuel
      recipe.input[getResourceNC("LiquidFuel")] = 100;
      recipe.output[getResource("energyMJ")] = 50;
      recipe.calculateNet();
    }
    {
      Recipe& recipe = recipes[getRecipe("compactedcoalenergy")];

      recipe.input[getResourceNC("CompactedCoal")] = 100;
      recipe.output[getResource("energyMJ")] = 20;
      recipe.calculateNet();
    }
    {
      Recipe& recipe = recipes[getRecipe("coalenergy")];
      recipe.input[getResourceNC("Coal")] = 100;
      recipe.output[getResource("energyMJ")] = 10;
      recipe.calculateNet();
    }


    //    print();

    fclose(fp);

    calculatePrecursors();
  }

  std::string convertBracesFormatToJsonish(std::string in) { // i am just not patient or good enough to get a decent parser+transform in only a few lines. Maybe one day. But not today.
    std::replace( in.begin(), in.end(), '"', '_');
    std::replace( in.begin(), in.end(), '\'', '_');
    std::replace( in.begin(), in.end(), '(', '{');
    std::replace( in.begin(), in.end(), ')', '}');
    std::replace( in.begin(), in.end(), '=', ':');

    std::regex e ("([{}:,])([^\"{}:,]{1,})([{}:,])");
    std::string result,result2,result3;
    std::regex_replace (std::back_inserter(result), in.begin(), in.end(), e, "$1\"$2\"$3");
    std::regex_replace (std::back_inserter(result2), result.begin(), result.end(), e, "$1\"$2\"$3");
    std::regex e2 ("[^\"]*Desc_([^\"]*)_C__");
    std::regex_replace (std::back_inserter(result3), result2.begin(), result2.end(), e2, "$1");
    result3[0] = '[';
    result3[result3.size() - 1] = ']';

    //prt(in);
    //    prt(result2);
    //prt(result3);
    //assert(false);
    return result3;
  }

  std::string shortenItemName(const std::string in) const {
    std::regex e ("Desc_(.*)_C");
    std::string shortinput;
    std::regex_replace (std::back_inserter(shortinput), in.begin(), in.end(), e, "$1");
    return shortinput;
  }

  void loadFromCommunityResourcesDocsJson(const std::string& jsonfilename){
    FILE* fp = fopen(jsonfilename.c_str(), "rb");
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    rapidjson::EncodedInputStream<rapidjson::UTF16LE<>, rapidjson::FileReadStream> eis(is);
    rapidjson::Document configdump;
    configdump.ParseStream<0, rapidjson::UTF16LE<> >(eis);
    assertss(configdump.IsArray(), " input is expected to be the <satisfactory-install>/communityresources/docs.json-file");


    for (const auto& partialconfig : configdump.GetArray()){
      if (std::string(partialconfig["NativeClass"].GetString()) == "Class'/Script/FactoryGame.FGBuildableManufacturer'"){
	for (const auto& producerjson :  partialconfig["Classes"].GetArray()){
	  powerconsumptionmw[ producerjson["ClassName"].GetString()] = std::stod(producerjson["mPowerConsumption"].GetString());
	}
      }
    }
    for (auto& power : powerconsumptionmw){
      prt2(power.first,power.second);
    }
    for (const auto& partialconfig : configdump.GetArray()){
      std::string nc(partialconfig["NativeClass"].GetString());
      if (nc == "Class'/Script/FactoryGame.FGItemDescriptor'" || nc == "Class'/Script/FactoryGame.FGResourceDescriptor'"){
	bool isresource = nc == "Class'/Script/FactoryGame.FGResourceDescriptor'";
	for (const auto& someitemjson :  partialconfig["Classes"].GetArray()){
	  Resource& r = resources[getResource(shortenItemName(someitemjson["ClassName"].GetString()))];
	  r.energyMJ = std::stod(someitemjson["mEnergyValue"].GetString());
	  r.isliquid = std::string(someitemjson["mForm"].GetString()) == "RF_LIQUID";
	  r.isresource = isresource;
	  prt4(r.name, r.isliquid, r.energyMJ, r.isresource);
	}
      }
    }
    for (const auto& partialconfig : configdump.GetArray()){
      if (std::string(partialconfig["NativeClass"].GetString()) == "Class'/Script/FactoryGame.FGBuildableGeneratorFuel'"){
	for (const auto& somegeneratorjson :  partialconfig["Classes"].GetArray()){
	  const std::string validfuels = somegeneratorjson["mDefaultFuelClasses"].GetString();
	  for (const auto& res : resources) {
	    if (res.energyMJ > 0 && validfuels.find("Desc_"  + res.name + "_C") != std::string::npos) {
	      //const std::string shortinput = shortenItemName(item_energyMJ.first);
	      Recipe& recipe = recipes[getRecipe("energy from " + res.name)];
	      recipe.input[res.index] = 100;
	      recipe.output[getResource("energyMJ")] = res.energyMJ * 100; // ??????
	      recipe.calculateNet();
	    }
	  }
	}
      }
    }



    for (const auto& partialconfig : configdump.GetArray()){
      if (std::string(partialconfig["NativeClass"].GetString()) == "Class'/Script/FactoryGame.FGRecipe'"){
	for (const auto& jsonrecipe :  partialconfig["Classes"].GetArray()){
	  const double durationminutes = std::stod(jsonrecipe["mManufactoringDuration"].GetString()) / 60.0;

	  Recipe& recipe = recipes[getRecipe(jsonrecipe["mDisplayName"].GetString())];
	  const std::string mingredients = convertBracesFormatToJsonish(jsonrecipe["mIngredients"].GetString());
	  rapidjson::Document ingredientsjson;
	  ingredientsjson.Parse(mingredients.c_str());
	  assertss(ingredientsjson.IsArray(),mingredients);
	  for (const auto& jsonrecipeinput : ingredientsjson.GetArray()  ) {
	    Resource& input = resources[getResource(jsonrecipeinput["ItemClass"].GetString())];
	    recipe.input[input.index] = std::stod(jsonrecipeinput["Amount"].GetString()) / durationminutes;
	    input.usedby.insert(recipe.index);
	  }
	  const std::string mproducts = convertBracesFormatToJsonish(jsonrecipe["mProduct"].GetString());
	  rapidjson::Document productsjson;
	  productsjson.Parse(mproducts.c_str());
	  assertss(productsjson.IsArray(),mproducts);
	  for (const auto& jsonrecipeoutput : productsjson.GetArray()  ) {
	    Resource& output = resources[getResource(jsonrecipeoutput["ItemClass"].GetString())];
	    recipe.output[output.index] = std::stod(jsonrecipeoutput["Amount"].GetString()) / durationminutes;
	    output.producedby.insert(recipe.index);
	  }

	  World::resourceindex zucht = getResource("_T"+ recipe.name);

	  std::string building = jsonrecipe["mProducedIn"].GetString();
	  for (const auto& producer : powerconsumptionmw){
	    if (building.find(producer.first) != std::string::npos) {
	      resources[zucht].isresource = true;
	      recipe.input[zucht] += 1;
	      
	      //TODO: ALSO ADD THE COOLING WATER, not sure how to get it from the json though.
	      recipe.input[getResourceNC("energyMJ")] = producer.second * 60.0; // watt is joule per second ... 60 seconds in a minute ... we are normalizing everything to minutes...
	      //	      assert(false);// please trigger.
	      break;
	    }
	  }

	  recipe.calculateNet();
	}
      }
    }

    //    print();

    fclose(fp);

  }



};

struct Simulation {
  World* w;
  //strategy
  std::vector<std::vector<World::recipeindex> > resource_to_waytoobtainit; // relevant recipes in order of priority

  struct RecipeStats {
    double runtime = 0;
    std::map<World::resourceindex,double> in, out;
  };

  struct ResourceStats {
    double total_demand = 0;
    std::map<World::recipeindex,double> sources, sinks;
  };

  // state of the simulator
  std::vector<double> resource_to_demand;
  // anti-loop-detection.
  std::set<Bits128> hashed_states;

  //statistics
  std::vector<ResourceStats> resource_stats;
  std::vector<RecipeStats> recipe_stats;


  // sankey 
  
//  std::vector<std::vector<World::resourceindex, double> > recipe_demands;

  Simulation(World* w_):
    w(w_),
    resource_to_waytoobtainit(w_->resources.size()),
    resource_to_demand(w_->resources.size(), 0),
    resource_stats(w_->resources.size()),
    recipe_stats(w->recipes.size()) {
    for (const auto& r : w->recipes) {
      for (const auto& i : r.neteffect) {
	if (i.second > 0){
	  resource_to_waytoobtainit[i.first].push_back(r.index); // default strategy, will be horrible inefficient, might not work even.
	}
      }
    }
  }

  void setDemand(const std::string& stuff, const double amount){
    const std::size_t r = w->getResourceNC(stuff);
    resource_to_demand[r] = amount;
    resource_stats[r].total_demand += amount;
  }

  void run() {

    // what about checking all the demands in one cycle ... and postponing adding the new demands till the next cycle ...
    double counter = 0;
    std::size_t maxcycles =  1000000;
    
    std::vector<World::resourceindex> allresources(resource_to_demand.size());
    for (World::resourceindex neededres = 0; neededres < resource_to_demand.size(); ++neededres) {
      allresources[neededres] = neededres;
    }
    
    while (counter++ < maxcycles ){
      assert(counter + 1 < maxcycles);

      std::map<World::resourceindex, double> additional_demands;
      //neededres = (neededres + 1) % resource_to_demand.size();
      //prt3(counter, maxcycles, neededres);
      std::random_shuffle(allresources.begin(), allresources.end());
      
      for (World::resourceindex neededres : allresources) {
	if (resource_to_demand[neededres] > 0.0001 && !w->resources[neededres].isresource) {
	  for (const World::recipeindex recipeindex : resource_to_waytoobtainit[neededres]) {
	    const World::Recipe& recipe = w->recipes[recipeindex];
	  
	    /*for (const auto& i : recipe.neteffect){
	      if (i.first == neededres){
	      prt3(w->resources[i.first] , i.second, recipe.name );
	      }
	      }*/
	  
	    const auto neti = recipe.neteffect.find(neededres);
	  
	  
	    if (neti != recipe.neteffect.end() && neti->second > 0){
	      //assert(false);
	      double runtime = std::numeric_limits<double>::max(); // this is based not on the available input but on the available demand.
	    
	    
	      double grr = 0;
	      for (const auto& o :  recipe.neteffect){
		if (o.second > 0){
		  const double maxpossible = resource_to_demand[o.first] / o.second;
		  if (runtime > maxpossible) {
		    runtime = maxpossible;
		  }
		  if (o.first == neededres) {
		    grr = o.second;
		  }
		}
	      }
	    
	      // trying to put a max on the allowed demand for created stuff.
	      if (false){
		for (const auto& in :  recipe.input){
		
		  //	prt2(w->resources[in.first].isresource,w->resources[in.first].name);
		  if (!w->resources[in.first].isresource){
		    double remaining_allowed_debt = w->resources[in.first].isliquid ? 100000 : 100.0;
		    remaining_allowed_debt -= resource_to_demand[in.first];
		    double maxpossible = remaining_allowed_debt / in.second;
		    if (maxpossible < 0) {
		      maxpossible = 0;
		    }
		    if (runtime > maxpossible) {
		      runtime = maxpossible;
		    }
		  }
		}
	      }
	      //TODO: every recipe with multiple outputs, make versions of that recipe where outputs are discarded
	      assert (grr > 0);
	      if (runtime  > grr && w->resources[neededres].name != "energyMJ") {
		//		assertss(false, grr);
		runtime = grr;
	      }
	    
	      assert(runtime < std::numeric_limits<double>::max());
	      if (runtime > 0) { // execute recipe.
		//runtime *= 0.9; // to allow other recipes to compete for the same... demand?
		// sqrt or log would be interesting here...
		if (w->resources[neededres].name != "energyMJ"){
		  if (runtime > 1000.0){ // small enough to get rid of heavyoilresidue, large enough to not slow the simulation too much
		    runtime = sqrt(runtime);
		  }else if (runtime > 10){ // small enough to get rid of heavyoilresidue, large enough to not slow the simulation too much
		    runtime /= 10.0;
		  }else if (runtime > 0.1) {
		    runtime = 0.1;
		  }
		  /*		  if (runtime < 0.01){
				  break;
				  }*/
		
		}
		std::cerr << "executing " << recipe.name << " X " << runtime  << " to satisfy demand of " << w->resources[neededres].name << " which is " << resource_to_demand[neededres]  << std::endl;
	      
		auto& recipestats = recipe_stats[recipeindex];
		recipestats.runtime += runtime;
		for (const auto& in :  recipe.input){
		  //const double before = resource_to_demand[in.first];
		
		  additional_demands[in.first] += in.second * runtime;
		  //assert(resource_to_demand[in.first] > -0.1);
		
		  recipestats.in[in.first] += in.second * runtime;
		  resource_stats[in.first].total_demand += in.second * runtime;
		  resource_stats[in.first].sinks[recipe.index] += in.second * runtime;
		
		  //prt3(before,resource_to_demand[in.first], w->resources[in.first]);
		
		}
		for (const auto& out :  recipe.output){
		
		  //const double before = resource_to_demand[out.first];
		  resource_to_demand[out.first] -= out.second * runtime;
		 // assertss(resource_to_demand[out.first] > -0.1,pt(resource_to_demand[out.first]));
		
		  recipestats.out[out.first] += out.second * runtime;
		  resource_stats[out.first].sources[recipe.index] += out.second * runtime;
		  // prt3(before,resource_to_demand[out.first],w->resources[out.first]);
		
		}
	      
		break; // otherwise the limit on the runtime per recipe will result in the activation of the next recipe which is probably undesired.
		//		done = false;
	      }
	    }
	    if (resource_to_demand[neededres] == 0){
	      break;
	    }
	  }
	}
      }      
      for (auto& moar : additional_demands){
	resource_to_demand[moar.first] += moar.second;
      }
      additional_demands.clear();
      for (auto& i: resource_to_demand) {
	assert(i > -0.1);
      }
      //if (neededres + 1 == w->resources.size())
      { // we iterated all the demands once more, find out if we should quit cycling.
	//static std::vector<float>  zucht(resource_to_demand.size());
	std::vector<float> demand_except_for_resources(resource_to_demand.size());
	for (const auto& r : w->resources) {
	  if (r.isresource){
	    demand_except_for_resources[r.index] = 0;
	    }else{
	    demand_except_for_resources[r.index] = (float)resource_to_demand[r.index];
	  }
	}
	/*	for (World::resourceindex i = 0; i < resource_to_demand.size(); i++){
		if (zucht[i] != demand_except_for_resources[i]){
		prt2(w->resources[i].name, demand_except_for_resources[i] - zucht[i]);
		}
		}
		zucht = demand_except_for_resources;*/
	Bits128 state_hash;
	MurmurHash3_x64_128(demand_except_for_resources.data(), demand_except_for_resources.size() * sizeof(float), 42,  &state_hash);
	if (hashed_states.find(state_hash) != hashed_states.end()){
	  return; // finished simulation, we had this state before.
	}
	hashed_states.insert(state_hash);
      }
      
    }
  }
};

//TODO: somehow the power recipes sometimes need water as input too, not sure how to get there.
//TODO: generate dot-graph
//TODO: generate that fancy sankey
//TODO: also add energy-usage for miners/pumps


int main() {
  // make a demand-driven satisfactory simulator.
  World w;
  //w.loadFromDerivedJson("recipes.json");
  w.loadFromCommunityResourcesDocsJson("Docsv0322.json");

  const auto createRecipeWithoutOutput = [&w]( const std::string& recipename, const std::string& resourcename) {
    World::Recipe& r = w.recipes[w.getRecipe(recipename + " no " + resourcename)];
    World::Recipe& original = w.recipes[w.getRecipeNC(recipename)];
    r.input = original.input;
    r.output = original.output;
    r.output.erase(w.getResourceNC(resourcename));
    r.calculateNet();
  };
  createRecipeWithoutOutput("Fuel","PolymerResin");
  createRecipeWithoutOutput("Alumina Solution","Silica");
  createRecipeWithoutOutput("Alternate: Electrode - Aluminum Scrap","Water");
  createRecipeWithoutOutput("Aluminum Scrap","Water");
  createRecipeWithoutOutput("Alternate: Heavy Oil Residue","PolymerResin");
  createRecipeWithoutOutput("Alternate: Polymer Resin","HeavyOilResidue");
    
  if(true){
    World::recipeindex ri =  w.concatenateRecipes(w.concatenateRecipes(w.getRecipeNC("Packaged Water"), w.getRecipeNC("Alternate: Diluted Packaged Fuel"), w.getResourceNC("PackagedWater"), "packwater&dilutewithheavy"), w.getRecipeNC("Unpackage Fuel"), w.getResourceNC("Fuel"), "unpackagedilutedfuel");
    World::resourceindex canisters = w.getResourceNC("FluidCanister");

    assert(w.recipes[ri].input[canisters] == w.recipes[ri].output[canisters]);
    w.recipes[ri].input.erase(canisters);
    w.recipes[ri].output.erase(canisters);
    w.recipes[ri].neteffect.erase(canisters);
  }
  if (true) {


    
    World::recipeindex heavyresidual =   
							      w.concatenateRecipes(
										   w.getRecipeNC("Alternate: Heavy Oil Residue"),
										   w.getRecipeNC("Residual Rubber"),
										   w.getResourceNC("PolymerResin"),
										   "heavyoilandrubber");
    World::recipeindex recyclerstorubber =   w.concatenateRecipes(w.getRecipeNC("Alternate: Recycled Plastic"), 
								  w.getRecipeNC("Alternate: Recycled Rubber"), 
								  w.getResourceNC("Plastic"), 
								  "recyclerstorubber");
    


// w.getRecipeNC("Alternate: Recycled Rubber"),w.getResourceNC("Plastic"), "supercrazyrubber"); 
//w.concatenateRecipes(

    World::recipeindex dtpi = w.concatenateRecipes(heavyresidual,
					      w.concatenateRecipes(w.getRecipeNC("unpackagedilutedfuel"),
								   recyclerstorubber, 
								   w.getResourceNC("LiquidFuel"),
					     "dilutingtorecyclerstorubber"),
					      w.getResourceNC("HeavyOilResidue"),
					      "danieltheprogrammer");
    auto& dtp = w.recipes[dtpi];
    
    for (auto& i : dtp.neteffect){
      if (i.second > 0) {
	dtp.input.erase(i.first);
	dtp.output[i.first] = i.second;
      }else if (i.second < 0) {
	dtp.output.erase(i.first);
	dtp.input[i.first] = -i.second;      
      }else{
	dtp.input.erase(i.first);
	dtp.output.erase(i.first);
      }
    }
    

/*    World::resourceindex removethis = w.getResourceNC("PolymerResin");
    assert(w.recipes[ri].input[removethis] == w.recipes[ri].output[removethis]);
    w.recipes[ri].input.erase(removethis);
    w.recipes[ri].output.erase(removethis);
    w.recipes[ri].neteffect.erase(canisters);*/
  }
if (true) {
  auto& recipe = w.recipes[w.getRecipe("free energy")];
  recipe.output[w.getResourceNC("energyMJ")] = 1000.0;
  recipe.calculateNet();
}

  w.resources[w.getResourceNC("CompactedCoal")].isresource = false;
  w.resources[w.getResourceNC("PackagedOil")].isresource = false;
  w.resources[w.getResourceNC("PackagedWater")].isresource = false;
  w.resources[w.getResourceNC("UraniumCell")].isresource = false;
  w.resources[w.getResourceNC("UraniumPellet")].isresource = false;

  w.print();
  //  assert(false);

  // ok euh... how do i iterate all these strategies ...
  Simulation s (&w);

s.resource_to_waytoobtainit[w.getResourceNC("Cement")]  = {
                //w.getRecipeNC("Concrete"),
                //w.getRecipeNC("Alternate: Rubber Concrete"),
                //w.getRecipeNC("Alternate: Wet Concrete"),
                w.getRecipeNC("Alternate: Fine Concrete"),
};
//s.setDemand("Cement",10000);

s.resource_to_waytoobtainit[w.getResourceNC("IronPlate")]  = {
                //w.getRecipeNC("Iron Plate"),
                //w.getRecipeNC("Alternate: Coated Iron Plate"),
                w.getRecipeNC("Alternate: Steel Coated Plate"),
};
//s.setDemand("IronPlate",10000);

s.resource_to_waytoobtainit[w.getResourceNC("IronRod")]  = {
                //w.getRecipeNC("Iron Rod"),
                w.getRecipeNC("Alternate: Steel Rod"),
};
//s.setDemand("IronRod",10000);

s.resource_to_waytoobtainit[w.getResourceNC("CrystalShard")]  = {
                w.getRecipeNC("Power Shard (1)"),
                w.getRecipeNC("Power Shard (2)"),
                w.getRecipeNC("Power Shard (5)"),
};
//s.setDemand("CrystalShard",1000);

s.resource_to_waytoobtainit[w.getResourceNC("Fuel")]  = {
                w.getRecipeNC("Alternate: Diluted Packaged Fuel"),
                w.getRecipeNC("Packaged Fuel"),
                w.getRecipeNC("packwater&dilutewithheavy"),
};
//s.setDemand("Fuel",1000);

s.resource_to_waytoobtainit[w.getResourceNC("Wire")]  = {
                //w.getRecipeNC("Wire"),
                w.getRecipeNC("Alternate: Fused Wire"),
                //w.getRecipeNC("Alternate: Iron Wire"),
                //w.getRecipeNC("Alternate: Caterium Wire"),
};
//s.setDemand("Wire",100000);

s.resource_to_waytoobtainit[w.getResourceNC("Cable")]  = {
                //w.getRecipeNC("Cable"),
                w.getRecipeNC("Alternate: Coated Cable"),
                //w.getRecipeNC("Alternate: Insulated Cable"),
                //w.getRecipeNC("Alternate: Quickwire Cable"),
};
//s.setDemand("Cable",10000);

s.resource_to_waytoobtainit[w.getResourceNC("IronPlateReinforced")]  = {
                //w.getRecipeNC("Reinforced Iron Plate"),
                //w.getRecipeNC("Alternate: Adhered Iron Plate"),
                //w.getRecipeNC("Alternate: Bolted Iron Plate"),
                w.getRecipeNC("Alternate: Stitched Iron Plate"),
};
//s.setDemand("IronPlateReinforced",1000);

s.resource_to_waytoobtainit[w.getResourceNC("CopperIngot")]  = {
                //w.getRecipeNC("Copper Ingot"),
                w.getRecipeNC("Alternate: Copper Alloy Ingot"),
                //w.getRecipeNC("Alternate: Pure Copper Ingot"),
};
//s.setDemand("CopperIngot",10000);

s.resource_to_waytoobtainit[w.getResourceNC("IronScrew")]  = {
                //w.getRecipeNC("Screw"),
                //w.getRecipeNC("Alternate: Casted Screw"),
                w.getRecipeNC("Alternate: Steel Screw"),
};
//s.setDemand("IronScrew",100000);

s.resource_to_waytoobtainit[w.getResourceNC("SpaceElevatorPart_1")]  = {
                w.getRecipeNC("Smart Plating"),
                //w.getRecipeNC("Alternate: Plastic Smart Plating"),
};
//s.setDemand("SpaceElevatorPart_1",1000);

s.resource_to_waytoobtainit[w.getResourceNC("SpaceElevatorPart_2")]  = {
                w.getRecipeNC("Alternate: Flexible Framework"),
                //w.getRecipeNC("Versatile Framework"),
};
//s.setDemand("SpaceElevatorPart_2",10);

s.resource_to_waytoobtainit[w.getResourceNC("SpaceElevatorPart_3")]  = {
                w.getRecipeNC("Alternate: Automated Speed Wiring"),
                //w.getRecipeNC("Automated Wiring"),
};
//s.setDemand("SpaceElevatorPart_3",100);

s.resource_to_waytoobtainit[w.getResourceNC("IronIngot")]  = {
                //w.getRecipeNC("Iron Ingot"),
                //w.getRecipeNC("Alternate: Pure Iron Ingot"),
                w.getRecipeNC("Alternate: Iron Alloy Ingot"),
};
//s.setDemand("IronIngot",1000);

s.resource_to_waytoobtainit[w.getResourceNC("Motor")]  = {
                //w.getRecipeNC("Motor"),
                w.getRecipeNC("Alternate: Rigour Motor"),
};
//s.setDemand("Motor",100);

s.resource_to_waytoobtainit[w.getResourceNC("CircuitBoard")]  = {
                //w.getRecipeNC("Circuit Board"),
                //w.getRecipeNC("Alternate: Electrode Circuit Board"),
                w.getRecipeNC("Alternate: Silicone Circuit Board"), // way cheaper and way less power
                //w.getRecipeNC("Alternate: Caterium Circuit Board"),
};
//s.setDemand("CircuitBoard",1000);

s.resource_to_waytoobtainit[w.getResourceNC("CopperSheet")]  = {
                w.getRecipeNC("Copper Sheet"),
                //w.getRecipeNC("Alternate: Steamed Copper Sheet"),
};
//s.setDemand("CopperSheet",1000);

s.resource_to_waytoobtainit[w.getResourceNC("Plastic")]  = {
                w.getRecipeNC("Plastic"),
                w.getRecipeNC("Alternate: Recycled Plastic"),
		//w.getRecipeNC("Residual Plastic"),
};
//s.setDemand("Plastic",1000);
// basically we have demand-driven simulators (this one) and input-driven simulators ........ ? the game does both depending on wether all buffers are full or empty...
s.resource_to_waytoobtainit[w.getResourceNC("Rubber")]  = {
		//w.getRecipeNC("danieltheprogrammer"),
                w.getRecipeNC("Rubber"),		
                w.getRecipeNC("Alternate: Recycled Rubber"),
		//w.getRecipeNC("Residual Rubber"),               
};
//s.setDemand("Rubber",100);


s.resource_to_waytoobtainit[w.getResourceNC("SteelPlateReinforced")]  = {
                //w.getRecipeNC("Encased Industrial Beam"),
                w.getRecipeNC("Alternate: Encased Industrial Pipe"),
};
//s.setDemand("SteelPlateReinforced",1000);

s.resource_to_waytoobtainit[w.getResourceNC("LiquidFuel")]  = {
                w.getRecipeNC("Fuel"),
                w.getRecipeNC("Fuel no PolymerResin"), // not bad either...
                //w.getRecipeNC("Residual Fuel"),
                //w.getRecipeNC("Unpackage Fuel"),
                w.getRecipeNC("unpackagedilutedfuel"), // water-heavy
};
//s.setDemand("LiquidFuel",1000000);

s.resource_to_waytoobtainit[w.getResourceNC("PolymerResin")]  = {
                w.getRecipeNC("Fuel"),
                w.getRecipeNC("Alternate: Heavy Oil Residue"),
                w.getRecipeNC("Alternate: Polymer Resin"),
                w.getRecipeNC("Alternate: Polymer Resin no HeavyOilResidue"),
};
//s.setDemand("PolymerResin",1000);

s.resource_to_waytoobtainit[w.getResourceNC("HeavyOilResidue")]  = {
                w.getRecipeNC("Plastic"),
                w.getRecipeNC("Rubber"),
                w.getRecipeNC("Alternate: Heavy Oil Residue"),
                w.getRecipeNC("Alternate: Polymer Resin"),
                w.getRecipeNC("Unpackage Heavy Oil Residue"),
                w.getRecipeNC("Alternate: Heavy Oil Residue no PolymerResin"),
};
//s.setDemand("HeavyOilResidue",1000000);

s.resource_to_waytoobtainit[w.getResourceNC("ModularFrame")]  = { // all very close ... mostly changes in power use...
               // w.getRecipeNC("Alternate: Bolted Frame"),
               // w.getRecipeNC("Modular Frame"),
                w.getRecipeNC("Alternate: Steeled Frame"),
};
//s.setDemand("ModularFrame",1000);

s.resource_to_waytoobtainit[w.getResourceNC("Rotor")]  = {
                w.getRecipeNC("Rotor"),   // not much copper but coal-heavy
                //w.getRecipeNC("Alternate: Copper Rotor"), // copper-heavy
                //w.getRecipeNC("Alternate: Steel Rotor"), //  
};
//s.setDemand("Rotor",1000);

s.resource_to_waytoobtainit[w.getResourceNC("SteelIngot")]  = {
                //w.getRecipeNC("Alternate: Coke Steel Ingot"),
                //w.getRecipeNC("Steel Ingot"),
                w.getRecipeNC("Alternate: Solid Steel Ingot"),
                //w.getRecipeNC("Alternate: Compacted Steel Ingot"),
};
//s.setDemand("SteelIngot",1000);

s.resource_to_waytoobtainit[w.getResourceNC("AluminaSolution")]  = {
								    w.getRecipeNC("Alumina Solution"),
								    w.getRecipeNC("Alumina Solution no Silica"),
};
//s.setDemand("AluminaSolution",10000);

s.resource_to_waytoobtainit[w.getResourceNC("AluminumScrap")]  = {
                w.getRecipeNC("Alternate: Electrode - Aluminum Scrap"),
                w.getRecipeNC("Alternate: Electrode - Aluminum Scrap no Water"),
                //w.getRecipeNC("Aluminum Scrap"),
                //w.getRecipeNC("Aluminum Scrap no Water"),
};
//s.setDemand("AluminumScrap",10000);

s.resource_to_waytoobtainit[w.getResourceNC("AluminumIngot")]  = {
                w.getRecipeNC("Aluminum Ingot"),
                //w.getRecipeNC("Alternate: Pure Aluminum Ingot"),
};
//s.setDemand("AluminumIngot",10000);

s.resource_to_waytoobtainit[w.getResourceNC("Silica")]  = {
                //w.getRecipeNC("Alumina Solution"),
                //w.getRecipeNC("Alternate: Cheap Silica"),
                w.getRecipeNC("Silica"),
};
//s.setDemand("Silica",10000);

s.resource_to_waytoobtainit[w.getResourceNC("Computer")]  = {
                //w.getRecipeNC("Computer"),
                //w.getRecipeNC("Alternate: Caterium Computer"),
                w.getRecipeNC("Alternate: Crystal Computer"),
};
//s.setDemand("Computer",100);

s.resource_to_waytoobtainit[w.getResourceNC("ModularFrameHeavy")]  = {
                //w.getRecipeNC("Alternate: Heavy Flexible Frame"),
                //w.getRecipeNC("Heavy Modular Frame"),
                w.getRecipeNC("Alternate: Heavy Encased Frame"),
};
//s.setDemand("ModularFrameHeavy",10);

s.resource_to_waytoobtainit[w.getResourceNC("GoldIngot")]  = {
                //w.getRecipeNC("Alternate: Pure Caterium Ingot"),
                w.getRecipeNC("Caterium Ingot"),
};
//s.setDemand("GoldIngot",1000);

s.resource_to_waytoobtainit[w.getResourceNC("HighSpeedConnector")]  = {
                w.getRecipeNC("Alternate: Silicone High-Speed Connector"),
                //w.getRecipeNC("High-Speed Connector"),
};
//s.setDemand("HighSpeedConnector",1000);

s.resource_to_waytoobtainit[w.getResourceNC("Stator")]  = {
                //w.getRecipeNC("Stator"),
                w.getRecipeNC("Alternate: Quickwire Stator"),
};
//s.setDemand("Stator",1000);

s.resource_to_waytoobtainit[w.getResourceNC("HighSpeedWire")]  = {
                w.getRecipeNC("Alternate: Fused Quckwire"),
                //w.getRecipeNC("Quickwire"),  // it doesnt matter much ... which of the two ...
};
//s.setDemand("HighSpeedWire",1000);

s.resource_to_waytoobtainit[w.getResourceNC("QuartzCrystal")]  = {
                //w.getRecipeNC("Alternate: Pure Quartz Crystal"),
                w.getRecipeNC("Quartz Crystal"),
};
//s.setDemand("QuartzCrystal",1000);

s.resource_to_waytoobtainit[w.getResourceNC("LiquidTurboFuel")]  = {
                w.getRecipeNC("Alternate: Turbo Heavy Fuel"),
                //w.getRecipeNC("Unpackage Turbo Fuel"),
               //w.getRecipeNC("Turbofuel"),
};
//s.setDemand("LiquidTurboFuel",100000);

s.resource_to_waytoobtainit[w.getResourceNC("FluidCanister")]  = {
                w.getRecipeNC("Unpackage Turbo Fuel"),
                w.getRecipeNC("Unpackage Liquid Biofuel"),
                w.getRecipeNC("Unpackage Fuel"),
                w.getRecipeNC("Unpackage Oil"),
                w.getRecipeNC("Unpackage Heavy Oil Residue"),
                w.getRecipeNC("Unpackage Water"),
                w.getRecipeNC("Empty Canister"),
};
//s.setDemand("FluidCanister",1000);
 
s.resource_to_waytoobtainit[w.getResourceNC("CrystalOscillator")]  = {
                w.getRecipeNC("Alternate: Insulated Crystal Oscillator"), // way less power here
                //w.getRecipeNC("Crystal Oscillator"),  // close ... a bit less water-heavy...
};
//s.setDemand("CrystalOscillator",100);

s.resource_to_waytoobtainit[w.getResourceNC("ElectromagneticControlRod")]  = {
                w.getRecipeNC("Alternate: Electromagnetic Connection Rod"),
                //w.getRecipeNC("Electromagnetic Control Rod"),
};
//s.setDemand("ElectromagneticControlRod",1000);

s.resource_to_waytoobtainit[w.getResourceNC("Gunpowder")]  = {
                w.getRecipeNC("Alternate: Fine Black Powder"),
                w.getRecipeNC("Black Powder"),
};
//s.setDemand("Gunpowder",1000);

s.resource_to_waytoobtainit[w.getResourceNC("AluminumPlateReinforced")]  = {
                w.getRecipeNC("Alternate: Heat Exchanger"),
                //w.getRecipeNC("Heat Sink"),
};
//s.setDemand("AluminumPlateReinforced",1000);

s.resource_to_waytoobtainit[w.getResourceNC("MotorLightweight")]  = {
                //w.getRecipeNC("Turbo Motor"),
                w.getRecipeNC("Alternate: Turbo Rigour Motor"),
};
//s.setDemand("MotorLightweight",22.5);

s.resource_to_waytoobtainit[w.getResourceNC("ModularFrameLightweight")]  = {
                w.getRecipeNC("Alternate: Radio Control System"),
                //w.getRecipeNC("Radio Control Unit"),
};
//s.setDemand("ModularFrameLightweight",100);

s.resource_to_waytoobtainit[w.getResourceNC("NobeliskExplosive")]  = {
                w.getRecipeNC("Alternate: Seismic Nobelisk"),
//                w.getRecipeNC("Nobelisk"),
};
//s.setDemand("NobeliskExplosive",1000);

s.resource_to_waytoobtainit[w.getResourceNC("Water")]  = {
                w.getRecipeNC("Alternate: Electrode - Aluminum Scrap"),
                w.getRecipeNC("Aluminum Scrap"),
                w.getRecipeNC("Unpackage Water"),
};
//s.setDemand("Water",10000);

s.resource_to_waytoobtainit[w.getResourceNC("Coal")]  = {
                w.getRecipeNC("Alternate: Charcoal"),
                w.getRecipeNC("Alternate: Biocoal"),
};
//s.setDemand("Coal",1000);

s.resource_to_waytoobtainit[w.getResourceNC("UraniumCell")]  = {
                //w.getRecipeNC("Encased Uranium Cell"),
                w.getRecipeNC("Alternate: Infused Uranium Cell"),
};
//s.setDemand("UraniumCell",1000);

s.resource_to_waytoobtainit[w.getResourceNC("energyMJ")]  = {
							     //w.getRecipeNC("free energy"),
                //w.getRecipeNC("energy from LiquidFuel"),
                w.getRecipeNC("energy from LiquidTurboFuel"),
                //w.getRecipeNC("energy from PetroleumCoke"),
                //w.getRecipeNC("energy from Coal"),
                //w.getRecipeNC("energy from CompactedCoal"),
};
//s.setDemand("energyMJ",1000000);

s.resource_to_waytoobtainit[w.getResourceNC("GenericBiomass")]  = {
                w.getRecipeNC("Biomass (Leaves)"),
                w.getRecipeNC("Biomass (Wood)"),
                w.getRecipeNC("Biomass (Alien Carapace)"),
                w.getRecipeNC("Biomass (Alien Organs)"),
                w.getRecipeNC("Biomass (Mycelia)"),
};
//s.setDemand("GenericBiomass",1000);

s.resource_to_waytoobtainit[w.getResourceNC("BlueprintGeneratedClass__/Game/FactoryGame/Resource/Equipment/Beacon/BP_EquipmentDescriptorBeacon.BP_EquipmentDescriptorBeacon_C__")]  = {
                w.getRecipeNC("Alternate: Crystal Beacon"),
                //w.getRecipeNC("Beacon"),
};
//s.setDemand("BlueprintGeneratedClass__/Game/FactoryGame/Resource/Equipment/Beacon/BP_EquipmentDescriptorBeacon.BP_EquipmentDescriptorBeacon_C__",1000);

s.resource_to_waytoobtainit[w.getResourceNC("Fabric")]  = {
                w.getRecipeNC("Alternate: Polyester Fabric"),
                //w.getRecipeNC("Fabric"),
};
//s.setDemand("Fabric",5);

s.resource_to_waytoobtainit[w.getResourceNC("NuclearFuelRod")]  = {
                //w.getRecipeNC("Nuclear Fuel Rod"),
                w.getRecipeNC("Alternate: Nuclear Fuel Unit"),
};
//s.setDemand("NuclearFuelRod",100);

s.resource_to_waytoobtainit[w.getResourceNC("LiquidBiofuel")]  = {
                w.getRecipeNC("Unpackage Liquid Biofuel"),
                w.getRecipeNC("Liquid Biofuel"),
};
//s.setDemand("LiquidBiofuel",1000);

s.resource_to_waytoobtainit[w.getResourceNC("Medkit")]  = {
                w.getRecipeNC("Medicinal Inhaler: Alien Organs"),
                w.getRecipeNC("Nutritional Inhaler"),
                w.getRecipeNC("Medicinal Inhaler"),
};
//s.setDemand("Medkit",1000);

//s.setDemand("LiquidFuel",1000000);

 s.setDemand("ModularFrameHeavy",10);
//s.setDemand("IronPlateReinforced",50);
//s.setDemand("Motor",10);
 //s.setDemand("MotorLightweight",10);
 s.setDemand("SpaceElevatorPart_5",10);
 s.setDemand("Battery",10);
 // s.setDemand("UraniumPellet",100);
 s.setDemand("CartridgeStandard",10);
 s.setDemand("NobeliskExplosive",10);
/*  for (const auto& res : w.resources){
    if (res.name.size() < 5){
      prt(res.name);
      s.setDemand(res.name, 10);
    }
  }*/

  //    s.setDemand("", 10);
//todo: redo ironplate and cable...
  s.run();

  // todo: also add "recipe" for power...
  //TODO: find the best solution for all the easy recipes, increment, accumulate, ...

  for (World::recipeindex i = 0; i < s.recipe_stats.size(); ++i) {
    const auto& stats  = s.recipe_stats[i];
    if (stats.runtime > 0) {
      std::cerr << string_format("used recipe %-45s %14.2f minutes(or buildings)", w.recipes[i].name.c_str(), stats.runtime  ) << std::endl;
      for (const auto& in: stats.in){
	if (w.resources[in.first].isliquid){
	  std::cerr << string_format("  used %14.2fm3 %-45s (%5.1f x 300 )\n",in.second / 1000.0,  w.resources[in.first].name.c_str(), in.second/ 1000.0 / 300.0 );
	}else{
	  std::cerr << string_format("  used %14.2f   %-45s (%5.1f x 780 )\n",in.second         ,  w.resources[in.first].name.c_str(), in.second / 780.0 );
	}
      }
      for (const auto& in: stats.out){
	if (w.resources[in.first].isliquid){
	  std::cerr << string_format("  made %14.2fm3 %-45s (%5.1f x 300 )\n",in.second / 1000.0,  w.resources[in.first].name.c_str(), in.second/ 1000.0 / 300.0 );
	}else{
	  std::cerr << string_format("  made %14.2f   %-45s (%5.1f x 780 )\n",in.second         ,  w.resources[in.first].name.c_str(), in.second / 780.0 );
	}
      }
    }
  }

//  std::map<World::resourceindex, Color> resource_to_color;
  auto colorify = [](World::resourceindex ri) -> Color {
		    unsigned int seed = ri;
		    
		    return hsv2rgb(              ((rand_r(&seed) % 1000) / 1000.0) ,
				     0.3 + 0.4 * ((rand_r(&seed) % 1000) / 1000.0) ,
				     0.8 + 0.2 * ((rand_r(&seed) % 1000) / 1000.0)

						 );
/*		    auto i = resource_to_color.find(ri);
		    if (i == resource_to_color.end()) {
		      i = resource_to_color.insert(std::make_pair(ri, hsv2rgb(0.7 * (double) resource_to_color.size(), 1.0, 0.99))).first;
		    }
		    return i->second;*/
		  };
// copy paste this stuff to https://observablehq.com/@mbostock/flow-o-matic for a sankey diagram
  for (const auto& res : w.resources) {
    const auto resstats = s.resource_stats[res.index];
    if (res.name == "energyMJ" || resstats.total_demand == 0 || (res.name.size() > 2 && res.name.substr(0,2) == "_T") ){
      continue; // skip this one, too difficult for sankey
    }

/*    if (resstats.sources.empty()) {

    }else if (resstats.sinks.empty()){
      
    }else{*/
      double accounted_produced = 0;
      double accounted_used = 0;
      for (auto& i : resstats.sources) {
	accounted_produced += i.second;
      }
      for (auto& i : resstats.sinks) {
	accounted_used += i.second;
      }

      for (auto& i : resstats.sources) {
	std::string sourcename = string_format("%s(%.0f)",w.recipes[i.first].name.c_str(),s.recipe_stats[i.first].runtime);
	for (auto& j : resstats.sinks) {
	  std::string sinkname = string_format("%s(%.0f)",w.recipes[j.first].name.c_str(),s.recipe_stats[j.first].runtime);
	  if (w.recipes[i.first].name.find("Recycled") != std::string::npos && w.recipes[j.first].name.find("Recycled") != std::string::npos ){
	    continue; //these 2 are circular, too difficult for sankey.
	  }
//Alternate: Electrode - Aluminum Scrap,Alumina Solution,1.53137,#00fc64
	  if (w.recipes[i.first].name == "Alternate: Electrode - Aluminum Scrap" && w.recipes[j.first].name == "Alumina Solution"){
	    continue;
	  }
	  if (w.recipes[i.first].name == "Uranium Pellet" && w.recipes[j.first].name == "Uranium Pellet"){
	    continue;
	  }
	  if (res.isliquid){
	    std::cerr << sourcename << "," << sinkname << "," << (i.second / 1000.0 / 300.0 *  j.second / resstats.total_demand    ) << "," << colorify(res.index) << std::endl;
	  }else{
	    std::cerr << sourcename << "," << sinkname << "," << (i.second / 780.0 *  j.second / resstats.total_demand) << "," << colorify(res.index) << std::endl;
	  }
	}
      }


      if (accounted_used >  1.0 + accounted_produced) {
	double mismatch = accounted_used - accounted_produced;
	for (auto& i : resstats.sinks) {
	  std::string sinkname = string_format("%s(%.0f)",w.recipes[i.first].name.c_str(),s.recipe_stats[i.first].runtime);
	  if (res.isliquid){
	    std::cerr << res.name << " pump" << "," << sinkname << "," << (mismatch / 1000.0 / 300.0  * i.second/accounted_used  ) << "," << colorify(res.index) << std::endl;
	  }else{
	    std::cerr << res.name << " miner" << "," << sinkname << "," << (mismatch / 780.0 * i.second/accounted_used ) << "," << colorify(res.index) << std::endl;
	  }
	}
      }else if (accounted_used + 1.0 < accounted_produced) {
	double mismatch = accounted_produced - accounted_used;
	for (auto& i : resstats.sources) {
	  std::string sourcename = string_format("%s(%.0f)",w.recipes[i.first].name.c_str(),s.recipe_stats[i.first].runtime);
	  if (res.isliquid){
	    std::cerr <<  sourcename << "," << res.name << " storage " << "," << (mismatch / 1000.0 / 300.0 * i.second/accounted_produced ) << "," << colorify(res.index) << std::endl;
	  }else{
	    std::cerr <<  sourcename << "," << res.name << " storage " << "," << (mismatch / 780.0 * i.second/accounted_produced) << "," << colorify(res.index) << std::endl;
	  }
	}
      

      }



    
//    assert(res.name != "Water");

  }

  double price = 0;
  double numfactories = 0;
  std::cerr << std::endl
	    << "resourcetype                                   demand                                    input" << std::endl
  	    << "------------------------------------------------------------------------------------------------" << std::endl;
  for (const auto& res : w.resources){
    const auto resstats =s.resource_stats[res.index]; 
    if (resstats.total_demand > 0.0001) {

      if (res.name == "energyMJ"){

	std::cerr << string_format(" %-40s %10.1f (%10.1f MW     ) %10.1f (%10.1f MW     )",
				   res.name.c_str(),
				   resstats.total_demand,
				   resstats.total_demand / 60.0,
				   s.resource_to_demand[res.index] ,
				   s.resource_to_demand[res.index] / 60.0 ) 
		  << std::endl;
	//	price += s.resource_to_demand[res.index] / 100;
	assert (std::fabs(s.resource_to_demand[res.index]) <  0.1);
      }else if (res.name.size() > 2 && res.name.substr(0,2) == "_T") {
	
	std::cerr << string_format(" %-40s %10.1f minutes (or buildings)",
				   res.name.c_str(),
				   resstats.total_demand
				   );
//	price += s.resource_to_demand[res.index] * 1.0;
	numfactories += s.resource_to_demand[res.index];
	std::cerr << std::endl;
      }else if (res.isliquid) {
	std::cerr << string_format(" %-40s %10.1f (%10.1f pipes  ) ",
				   (res.name + "(m3)").c_str(),
				   resstats.total_demand / 1000.0,
				   resstats.total_demand / 1000.0 / 300.0
				   );
	price += s.resource_to_demand[res.index] / 1000.0 / 3.0;

	assertss(res.isresource || std::fabs(s.resource_to_demand[res.index]) < 0.1, std::fabs(s.resource_to_demand[res.index]));

	if (s.resource_to_demand[res.index] > 0.001){
	  std::cerr << string_format("%10.1f (%10.1f pipes  )",
				     s.resource_to_demand[res.index] / 1000.0 ,
				     s.resource_to_demand[res.index] / 1000.0 / 300.0
				     );
	}
	std::cerr << std::endl;
      }else{
	std::cerr << string_format(" %-40s %10.1f (%10.1f convmk5) ",
				   res.name.c_str(),
				   resstats.total_demand,
				   resstats.total_demand / 780.0
				   );
	price += s.resource_to_demand[res.index] / 7.8;
	assert(res.isresource || std::fabs(s.resource_to_demand[res.index]) < 0.1);
	if (s.resource_to_demand[res.index] > 0.001){
	  std::cerr << string_format("%10.1f (%10.1f convmk5)",
				     (s.resource_to_demand[res.index] ),
				     (s.resource_to_demand[res.index] / 780.0)
				     );
	}
	std::cerr << std::endl;
      }
    }
  }

  prt3(price, numfactories, sqrt(price * numfactories) );

  if (false){
    for (auto& res : w.resources){
      int amount = 0;
      std::ostringstream oss;
      oss <<   "s.resource_to_waytoobtainit[w.getResourceNC(\"" << res.name << "\")]  = {" << std::endl;
      for (auto& rec: w.recipes) {
	auto i = rec.neteffect.find(res.index);
	if (i != rec.neteffect.end() && i->second > 0){
	  oss << "\t\tw.getRecipeNC(\""<< rec.name <<  "\")," << std::endl;
	  amount++;
	}
      }
      oss << "};"  << std::endl
	  << "s.setDemand(\"" << res.name << "\",1000);" << std::endl
	  << std::endl;
      if (amount > 1){
	std::cerr << oss.str();
      }
    }
  }


};
