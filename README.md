# satisfactorysimulator
Simulator for the game satisfactory. Requires a dump from the game-configuration.
No attempt has been made to made to make this polished or user-friendly.

Dependencies: rapidjson, c++-11


Structure of main, edit this at your convenience:
int main(){
    // parse and load the configuration: all resources&items, all recipes and all the power generating buildings.
    ...
    // add additional recipes to help fix problems: variations on existing recipes where one of the outputproducts is discarded.

    createRecipeWithoutOutput("Fuel","PolymerResin"); // as if the polymerressin will be fed to the shredder.
    ...
    
    // Prepare a simulation with a strategy: for each resource there will be a demand and a customised sequence of recipes to obtain it.
    // To satisfy demand the sequence of recipes will be iterated, the first one that can be used will be executed for a limited amount.
    Simulation s (&w);
    s.resource_to_waytoobtainit[w.getResourceNC("Cement")]  = {
                //w.getRecipeNC("Concrete"),
                //w.getRecipeNC("Alternate: Rubber Concrete"),
                //w.getRecipeNC("Alternate: Wet Concrete"),
                w.getRecipeNC("Alternate: Fine Concrete"),
    };
    ...

    //	Then the initial demand is set. All the simulation starts from there. By default all demand is 0.
    s.setDemand("MotorLightweight",10);
    ...

    // run the simulation. During the simulation lots of statistics will be kept.
    s.run(); // internally there is a while-loop that goes on till the state doesnt change anymore (or is in a loop)

    // now there are various ways of getting those statistics.
    // - just dump all recipes and what they did.
    // - dump all resources and what they did.
    // - dump each flow from one recipe to another, ideal for sankey-diagrams.
    //    --> part of the output can be copied straight to https://observablehq.com/@mbostock/flow-o-matic 

}



