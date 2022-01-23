const fs = require('fs');
const inflection = require('inflection');
const { stringify } = require('querystring');

const underscore = (string) => {
  return inflection.underscore(
    string
      .replace(/(\d)([A-Z])/g, (m, f, rest) => f + rest.toLowerCase())
      .replace(/([A-Z])([A-Z]+)/g, (m, f, rest) => f + rest.toLowerCase())
  )
  .replace(/__/g, '_')
  .replace(/([a-z])_(\d)/g, '$1$2')
}

const xml = fs.readFileSync(process.argv[2]).toString();
const od = fs.readFileSync(process.argv[3]).toString();

const types = {}
const files = {};
const structs = {};

const findDefinition = (uid) => {
  const re = new RegExp(`<q1:parameter uniqueID="${uid}"[^>]*?>([\\s\\S]*?)</q1:parameter>`, 'g')
  const match = re.exec(xml)
  if (match) {
    const labelRe = new RegExp(`<CANopenObject[^>]*?index="${uid.replace(/[^\d]/g, '')}"[^>]*?name="([^"]+?)"`, 'g')
    const label = labelRe.exec(xml);
    return {
      description: (match[1].match(/<description[^>]+>(.*?)<\/description>/) || [null, null])[1],
      label: label ? label[1] : null
    }
  }
  return {}
}

var toHex = function (rgb) { 
  var hex = Number(rgb).toString(16);
  if (hex.length < 2) {
       hex = "0" + hex;
  }
  return hex;
};

od.replace(/\{\s*([^}]+?)\s*\}[^}]+?x([3-9].*?)_([a-z]+)([A-Z][^_\s,;]+)/g, (match, struct, index, type, name) => {
  struct = struct.replace(/\n\s+/g, '\n    ')
  struct = struct.replace(/( )([^; ]+);/g, (m, w, v) => w + underscore(v) + ';')
  struct = struct.replace(/[^;]+;(?:\n|$)/, ''); // remove highest index entry
  name = underscore(name);


  const definition = findDefinition(`UID_OBJ_${index}`);

  var subtype = 'config'

  const accessors = [];
  const prototypes = [];
  const defs = [];

  const typeName = type == 'core' ? `${name}_${subtype}` : `${type}_${name}_${subtype}`
  const structName = `${type}_${name}`
  const filePath = `./src/${type}/${name}.h`;
  if (type != 'core')
    structs[structName] = definition.description || '';

  struct.split(/;/).map((pair, attributeIndex) => {
    if (!pair) return;
    const [dataType, attribute] = pair.trim().split('[')[0].split(' ')
    
    
    
    


    const constant = `SUBIDX_${name.toUpperCase()}_${attribute.toUpperCase()}`
    defs.push(`#define ${constant} 0x${(attributeIndex + 1)}`);
    const attributeOD = `${index.replace(/\d\d$/, 'XX')}${toHex(attributeIndex + 1)}`
    const attributeDefinition = findDefinition(`UID_SUB_${attributeOD}`);
    const shorttype = dataType.replace(/_t/, '').replace(/([a-z])[a-z]+/, '$1');
    if (attributeDefinition.description) {
      struct = struct.replace(' ' + attribute + ';', ' ' + attribute + '; // ' + attributeDefinition.description + ' ')
    }
    if (subtype != 'config') {
      
  prototypes.push(
`
ODR_t ${type}_${name}_set_${attribute}(${type}_${name}_t *${name}, ${dataType} value); // 0x${attributeOD}: ${name} ${subtype} ${attribute}
${dataType} ${type}_${name}_get_${attribute}(${type}_${name}_t *${name}); // 0x${attributeOD}: ${name} ${subtype} ${attribute}`)

accessors.push(`OD_ACCESSORS(${type}, ${name}, ${subtype}, ${attribute}, ${constant}, ${dataType}, ${shorttype}) /* 0x${attributeOD}: ${subtype} */`);

if (!'USE MACROS?')    accessors.push(
`/* 0x${attributeOD}: ${name} ${subtype} ${attribute} */
weak ODR_t ${type}_${name}_set_${attribute}(${type}_${name}_t *${name}, ${dataType} value) {
  return OD_set_${shorttype}(${name}->device->${subtype}, ${constant}, value, false);
}
weak ${dataType} ${type}_${name}_get_${attribute}(${type}_${name}_t *${name}) {
  ${dataType} value;
  OD_get_${dataType.replace(/_t/, '').replace(/([a-z])[a-z]+/, '$1')}(${name}->device->${subtype}, ${constant}, &value, false);
  return value; 
}`)
} /**
ODR_t ${name}_read_${attribute}(${type}_${name}_t *${name}, ${dataType} value) {
  return OD_set_${dataType.replace(/_t/, '').replace(/([a-z])[a-z]+/, '$1')}(${name}->device->${subtype}, ${constant}, value, false);
} */
  }).join('\n')


  try {
      
    if (!types[typeName]) {
      types[typeName] = true;
      if (!files[filePath]) files[filePath] = { types: [], accessors: [], prototypes: [], defs: [] };
      files[filePath].types.push(`/* 0x${index}: ${definition.label}${definition.description && '\n   ' + definition.description} */\ntypedef struct ${typeName} {\n    uint8_t parameter_count;\n${struct}\n} ${typeName}_t;`)
      files[filePath].defs.push(defs.join('\n'))
      files[filePath].accessors.push(accessors.join('\n'))
      files[filePath].prototypes.push(prototypes.join(''))
      files[filePath].struct = structName;
    }
  } catch(e) {
    console.log(e.toString());
  }


})


for (var [filePath, value] of Object.entries(files)) {
  try {
    var oldHeader = fs.readFileSync(filePath).toString()
    if (oldHeader) {
      oldHeader = oldHeader.replace(/(\/\*\s*Start of autogenerated OD types\s*\*\/)[\s\S]*(\/\*\s*End of autogenerated OD types\s*\*\/)/, (m, s, e) => {
        return s + `\n${value.types.join('\n\n')}\n`+ e;
      })
      oldHeader = oldHeader.replace(/(\/\*\s*Start of autogenerated OD accessors\s*\*\/)[\s\S]*(\/\*\s*End of autogenerated OD accessors\s*\*\/)/, (m, s, e) => {
        return s + `\n${value.defs.join('\n')}\n${value.prototypes.join('')}\n`+ e;
      })
      fs.writeFileSync(filePath, oldHeader);
    }
    var oldC = fs.readFileSync(filePath.replace('.h', '.c')).toString()
    if (oldC) {
      oldC = oldC.replace(/(\/\*\s*Start of autogenerated OD accessors\s*\*\/)[\s\S]*(\/\*\s*End of autogenerated OD accessors\s*\*\/)/, (m, s, e) => {
        return s + `\n${value.accessors.filter(Boolean).join('\n')}\n`+ e;
      })
      console.log(filePath.replace('.h', '.c'))
      fs.writeFileSync(filePath.replace('.h', '.c'), oldC);
    }
  } catch(e) {
    console.log(e.toString());
  }
}

// update types.h with forward definitions of structs
var typeH = fs.readFileSync('./src/core/types.h').toString()
typeH = typeH.replace(/(\/\*\s*Start of autogenerated OD types\s*\*\/)[\s\S]*(\/\*\s*End of autogenerated OD types\s*\*\/)/, (m, s, e) => {
  const defs = Object.keys(structs).map((struct) => {
    return `typedef struct ${struct} ${struct}_t /* ${structs[struct]}*/;`
  })
  return s + `\n${defs.join('\n')}\n`+ e;
})
fs.writeFileSync('./src/core/types.h', typeH);
