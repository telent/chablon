# adapted from elftools example: dwarf_die_tree.py

from __future__ import print_function
import sys
import json

from elftools.elf.elffile import ELFFile
import arpy
import pdb

# C namespaces:
# - labels
# - tags (structs/unions/enum names)
# - members of structs/unions (one ns per struct/union)
# - other identifiers (functions, objects, typedefs, enum constants etc)
tags = {}
identifiers = {}

unnamed_count = 1
def unnamed():
    global unnamed_count
    unnamed_count += 1
    return "unnamed_%d" % unnamed_count

def name_of(die):
    if die.tag in ["DW_TAG_structure_type", "DW_TAG_union_type"]:
        tag  = 'tag'
    else:
        tag = 'identifier'

    if 'DW_AT_name' in die.attributes:
        return {tag: die.attributes['DW_AT_name'].value.decode('ascii')}
    else:
        return None

def find_type(die):
    global tags
    global identifiers
    if die.tag == 'DW_TAG_array_type':
        count = None
        for child in die.iter_children():
            if child.tag == 'DW_TAG_subrange_type':
                if 'DW_AT_upper_bound' in child.attributes:
                    count = child.attributes['DW_AT_upper_bound'].value
                elif 'DW_AT_count' in child.attributes:
                    count = child.attributes['DW_AT_count'].value
                else:
                    pdb.set_trace()
        # for number of elements,
        # find child called DW_TAG_subrange_type
        # and get DW_AT_upper_bound and add 1
        return {
            # XXX number of elements would be handy
            'array': {
                'count': count,
                'of': find_type(die.get_DIE_from_attribute('DW_AT_type'))
            }
        }
    elif die.tag == 'DW_TAG_base_type':
        return  {
            'bytes': die.attributes['DW_AT_byte_size'].value,
            'name': die.attributes['DW_AT_name'].value.decode('ascii')
        }
    elif die.tag == 'DW_TAG_typedef':
        typedef = walk_typedef(die)
        identifiers = { **identifiers, **typedef }
        n = [*typedef][0]
        bytes = None
        if 'bytes' in typedef[n]:
            bytes = typedef[n]['bytes']
        return { 'typedef': n, 'bytes': bytes }
    elif die.tag == 'DW_TAG_pointer_type':
        ptr_to = None
        if 'DW_AT_type' in die.attributes:
            ptr_to = name_of(die.get_DIE_from_attribute('DW_AT_type'))
        ptr_to = ptr_to or "void"
        return  {
            'bytes': die.attributes['DW_AT_byte_size'].value,
            # we'd like to say what it's a pointer to, but that
            # may involve recursively descending a struct, so
            # have to think about that
            'pointer': ptr_to
        }
    elif die.tag in ['DW_TAG_structure_type', 'DW_TAG_union_type']:
        kw = { 'DW_TAG_structure_type': 'struct',
               'DW_TAG_union_type': 'union' }[die.tag]
        struct = walk_struct_or_union(die)
        name = next(iter(struct.keys()))
        if name == None:
            return { 'include': struct }
        else:
            tags = { **tags, **struct }
            return { kw:  name }
    elif die.tag == 'DW_TAG_volatile_type':
        return {
            **find_type(die.get_DIE_from_attribute('DW_AT_type')),
            **{'volatile': True}
        }
    else:
        pdb.set_trace()

def walk_typedef(die):
    typ = find_type(die.get_DIE_from_attribute('DW_AT_type'))
    name = die.attributes['DW_AT_name'].value.decode('ascii')
    #pdb.set_trace()
    return { name: typ }

def inc_offsets(members, increment):
    def bump(v):
        return {**v, **{'offset': v['offset'] + (increment or 0)}}
    return dict(map(lambda kv: (kv[0], bump(kv[1])), members.items()))

def walk_struct_members(die):
    m = {}
    for child in die.iter_children():
        attr = child.attributes
        if (child.tag == 'DW_TAG_member') and ('DW_AT_name' in attr):
            offset = None
            if 'DW_AT_data_member_location' in attr:
                offset = attr['DW_AT_data_member_location'].value
            typ = find_type(child.get_DIE_from_attribute('DW_AT_type'))
            n = next(iter(typ.keys()))
            if n == 'include':
                included = inc_offsets(typ['include'][None]['members'], offset)
                m = { **m, **included }
            else:
                m[attr['DW_AT_name'].value.decode('ascii')] = {
                    'offset': offset,
                    'type': typ
                }
    return m

def walk_struct_or_union(die):
    attr =  die.attributes
    value = { 'members': walk_struct_members(die) }
    if 'DW_AT_byte_size' in attr:
        value['bytes'] = attr['DW_AT_byte_size'].value
    if 'DW_AT_name' in attr:
        name = attr['DW_AT_name'].value.decode('ascii')
        return { name: value }
    else:
        return { None: value }


def walk_die_info(die):
    global tags
    if die.tag in ["DW_TAG_structure_type", "DW_TAG_union_type"]:
        tags = { **tags, **walk_struct_or_union(die) }

    for child in die.iter_children():
        walk_die_info(child)

def process_stream(f):
    elffile = ELFFile(f)

    if not elffile.has_dwarf_info():
        print('  file has no DWARF info')
        return

    for CU in elffile.get_dwarf_info().iter_CUs():
        walk_die_info(CU.get_top_DIE())

if __name__ == '__main__':
    with arpy.Archive(sys.argv[1]) as ar:
        # print("files: %s" % ar.namelist(), file=sys.stderr)
        for header in (ar.infolist()[17:21]):
            print("file: %s" % header.name, file=sys.stderr)
            with ar.open(header) as f:
                process_stream(f)
    json.dump({
        'tag': tags,
        'identifier': identifiers
    }, sys.stdout)
