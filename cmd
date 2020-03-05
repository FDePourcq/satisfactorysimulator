cat satisfactoryDocs3.json | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[].ClassName '


cat satisfactoryDocs3.json | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | {mDisplayName,mIngredients,mProduct} ' 


cat satisfactoryDocs3.json | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | {mDisplayName,mIngredients,mProduct} ' | sed 's/([^(]*Desc_/(/g;s/_C...,/,/g' 



cat satisfactoryDocs3.json | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | select(.mProduct | contains("Resource/Parts")) | {mDisplayName,mIngredients,mProduct}'


cat satisfactoryDocs3.json | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | select(.mProduct | contains("Resource/Parts")) | {mDisplayName,mIngredients,mProduct}' | sed 's/ItemClass=[^.,]*\.\([^.,]*\)...,Amount/ItemClass=\1,Amount/g;s/Desc_//g;s/_C,/,/g'


cat satisfactoryDocs3.json | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | select(.mProduct | contains("Resource/Parts")) | {mDisplayName,mIngredients,mProduct}' | sed 's/ItemClass=[^.,]*\.\([^.,]*\)...,Amount/ItemClass=\1,Amount/g;s/Desc_//g;s/_C,/,/g' | sed 's/(/{/g;s/)/}/g;s/=/:/g;s/\"{{/{{/g;s/}}\"/}}/g'


cat satisfactoryDocs3.json | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | select(.mProduct | contains("Resource/Parts")) | {mDisplayName,mIngredients,mProduct}' | sed 's/ItemClass=[^.,]*\.\([^.,]*\)...,Amount/ItemClass=\1,Amount/g;s/Desc_//g;s/_C,/,/g' | sed 's/(/{/g;s/)/}/g;s/=/:/g;s/\"{{/[{/g;s/}}\"/}]/g;s/^}$/},/g'  | awk 'BEGIN{ print "["}{print $0}END{print "]"}'


cat satisfactoryDocs3.json | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | select(.mProduct | contains("Resource/Parts")) | {mDisplayName,mIngredients,mProduct}' | sed 's/ItemClass=[^.,]*\.\([^.,]*\)...,Amount/ItemClass=\1,Amount/g;s/Desc_//g;s/_C,/,/g' | sed 's/(/{/g;s/)/}/g;s/=/:/g;s/\"{{/[{/g;s/}}\"/}]/g;s/^}$/},/g;s/:\([^,}\"]*\)}/:"\1"}/g;s/:\([^,}[{\"]*\),/:"\1",/g'  | awk 'BEGIN{ print "["}{print $0}END{print "]"}'   > blaat.json


cat satisfactoryDocs3.json | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | select(.mProduct | contains("Resource/Parts")) | {mDisplayName,mIngredients,mProduct}' | sed 's/ItemClass=[^.,]*\.\([^.,]*\)...,Amount/ItemClass=\1,Amount/g;s/Desc_//g;s/_C,/,/g' | sed 's/(/{/g;s/)/}/g;s/=/:/g;s/\"{{/[{/g;s/}}\"/}]/g;s/^}$/},/g;'  | awk 'BEGIN{ print "["}{print $0}END{print "]"}'   | sed 's/\([{,]\)\([^\",}:]*\):\([^\",}:]*\)\([},]\)/ \1"\2":"\3"\4/g'  | sed 's/\([{,]\)\([^\",}:]*\):\([^\",}:]*\)\([},]\)/ \1"\2":"\3"\4/g'


cat satisfactoryDocs3.json | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | select(.mProduct | contains("Resource/Parts")) | {mDisplayName,mIngredients,mProduct}' | sed 's/ItemClass=[^.,]*\.\([^.,]*\)...,Amount/ItemClass=\1,Amount/g;s/Desc_//g;s/_C,/,/g' | sed 's/(/{/g;s/)/}/g;s/=/:/g;s/\"{{/[{/g;s/}}\"/}]/g;s/^}$/},/g;'  | awk 'BEGIN{ print "["}{print $0}END{print "]"}'   | sed 's/\([{,]\)\([^\",{}:]*\):\([^\",{}:]*\)\([},]\)/ \1"\2":"\3"\4/g'  | sed 's/\([{,]\)\([^\",{}:]*\):\([^\",{}:]*\)\([},]\)/ \1"\2":"\3"\4/g'


cat satisfactoryDocs3.json | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | select(.mProduct | contains("Resource/Parts")) | {mDisplayName,mIngredients,mProduct}' | sed 's/ItemClass=[^.,]*\.\([^.,]*\)...,Amount/ItemClass=\1,Amount/g;s/Desc_//g;s/_C,/,/g' | sed 's/(/{/g;s/)/}/g;s/=/:/g;s/\"{{/[{/g;s/}}\"/}]/g;'    | sed 's/\([{,]\)\([^\",{}:]*\):\([^\",{}:]*\)\([},]\)/ \1"\2":"\3"\4/g'  | sed 's/\([{,]\)\([^\",{}:]*\):\([^\",{}:]*\)\([},]\)/ \1"\2":"\3"\4/g' | awk 'BEGIN{ print "["}{ if ($0=="{" && NR > 1) { print ",{";  }else{ print $0;  }    }END{print "]"}'


cat satisfactoryDocs3.json | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | select(.mProduct | contains("Resource/Parts")) | {mDisplayName,mIngredients,mProduct}' | sed 's/ItemClass=[^.,]*\.\([^.,]*\)...,Amount/ItemClass=\1,Amount/g;s/Desc_//g;s/_C,/,/g' | sed 's/(/{/g;s/)/}/g;s/=/:/g;s/\"{{/[{/g;s/}}\"/}]/g;'    | sed 's/\([{,]\)\([^\",{}:]*\):\([^\",{}:]*\)\([},]\)/ \1"\2":"\3"\4/g'  | sed 's/\([{,]\)\([^\",{}:]*\):\([^\",{}:]*\)\([},]\)/ \1"\2":"\3"\4/g' | awk 'BEGIN{ print "["}{ if ($0=="{" && NR > 1) { print ",{";  }else{ print $0;  }    }END{print "]"}'  > recipes.json

iconv -f UTF-16 -t UTF-8 satisfactoryDocs.json  -o - | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | select(.mProduct | contains("Resource/Parts")) | {mDisplayName,mIngredients,mProduct}' | sed 's/ItemClass=[^.,]*\.\([^.,]*\)...,Amount/ItemClass=\1,Amount/g;s/Desc_//g;s/_C,/,/g' | sed 's/(/{/g;s/)/}/g;s/=/:/g;s/\"{{/[{/g;s/}}\"/}]/g;'    | sed 's/\([{,]\)\([^\",{}:]*\):\([^\",{}:]*\)\([},]\)/ \1"\2":"\3"\4/g'  | sed 's/\([{,]\)\([^\",{}:]*\):\([^\",{}:]*\)\([},]\)/ \1"\2":"\3"\4/g' | awk 'BEGIN{ print "["}{ if ($0=="{" && NR > 1) { print ",{";  }else{ print $0;  }    }END{print "]"}'  | jq . > recipes.json


iconv -f UTF-16 -t UTF-8 satisfactoryDocs.json  -o - | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | select(.mProduct | contains("Resource/Parts")) | {mDisplayName,mIngredients,mProduct,mManufactoringDuration,mProducedIn}' | sed 's/ItemClass=[^.,]*\.\([^.,]*\)...,Amount/ItemClass=\1,Amount/g;s/Desc_//g;s/_C,/,/g' | sed 's/(/{/g;s/)/}/g;s/=/:/g;s/\"{{/[{/g;s/}}\"/}]/g;'    | sed 's/\([{,]\)\([^\",{}:]*\):\([^\",{}:]*\)\([},]\)/ \1"\2":"\3"\4/g'  | sed 's/\([{,]\)\([^\",{}:]*\):\([^\",{}:]*\)\([},]\)/ \1"\2":"\3"\4/g' | awk 'BEGIN{ print "["}{ if ($0=="{" && NR > 1) { print ",{";  }else{ print $0;  }    }END{print "]"}'  | jq .


iconv -f UTF-16 -t UTF-8 satisfactoryDocs.json  -o - | jq '.[] | select(.NativeClass | contains("FGRecipe") ) | .Classes[]  | select(.mProduct | contains("Resource/Parts")) | {mDisplayName,mIngredients,mProduct,mManufactoringDuration,mProducedIn}' | sed 's/ItemClass=[^.,]*\.\([^.,]*\)...,Amount/ItemClass=\1,Amount/g;s/Desc_//g;s/_C,/,/g' | sed 's/(/{/g;s/)/}/g;s/=/:/g;s/\"{{/[{/g;s/}}\"/}]/g;'    | sed 's/\([{,]\)\([^\",{}:]*\):\([^\",{}:]*\)\([},]\)/ \1"\2":"\3"\4/g'  | sed 's/\([{,]\)\([^\",{}:]*\):\([^\",{}:]*\)\([},]\)/ \1"\2":"\3"\4/g' | awk 'BEGIN{ print "["}{ if ($0=="{" && NR > 1) { print ",{";  }else{ print $0;  }    }END{print "]"}'  | sed 's/\"[^\"]*Build_\([a-zA-Z0-9]*\).*\}\"/\"\1\"/g' | jq .




inkscape -z -e test.png  brightness11.svg && convert test.png -flatten test2.png && feh test2.png

inkscape -z -e test.png  brightness11.svg && convert test.png -background white -alpha remove -alpha off test2.png && feh test2.png

convert test.svg -background white -alpha remove -alpha off test2.png && feh test2.png