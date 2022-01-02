# adapted from elftools example dwarf_die_tree.py

from __future__ import print_function
import sys
import json

from elftools.elf.elffile import ELFFile
import io
import arpy
import pdb

# C namespaces:
# - labels
# - tags (structs/unions/enum names)
# - members of structs/unions (one ns per struct/union)
# - other identifiers (functions, objects, typedefs, enum constants etc)
names = {
    'tag': {},
    'identifier': {}
}

def die_name(die):
    if 'DW_AT_name' in die.attributes:
        return die.attributes['DW_AT_name'].value.decode('ascii')
    else:
        return None

def die_namespace(die):
    if die.tag in ["DW_TAG_structure_type", "DW_TAG_union_type"]:
        return 'tag'
    else:
        return 'identifier'

def die_type(die):
    if 'DW_AT_type' in die.attributes:
        return die.get_DIE_from_attribute('DW_AT_type')
    else:
        return None


def chase_die(die):
    if die and die.tag in ['DW_TAG_volatile_type', 'DW_TAG_const_type']:
        return chase_die(die_type(die))
    else:
        return die

def array_count(die):
    count = None
    for child in die.iter_children():
        if child.tag == 'DW_TAG_subrange_type':
            if 'DW_AT_upper_bound' in child.attributes:
                count = child.attributes['DW_AT_upper_bound'].value
            elif 'DW_AT_count' in child.attributes:
                count = child.attributes['DW_AT_count'].value
            else:
                pdb.set_trace()
    return count

def reference(die):
    if die_type(die) == None:
        return { 'kind': 'base', 'type': 'void' };
    ref_die = chase_die(die_type(die))

    if die_name(ref_die):
        record_name(ref_die)
        ref = {
            die_namespace(ref_die): die_name(ref_die)
        }
    else:
        ref = parse_die(ref_die)
    return ref

def parse_die(die):
    if die.tag == 'DW_TAG_array_type':

        return {
            'kind': 'array',
            'count': array_count(die),
            'of': reference(die)
        }
    elif die.tag == 'DW_TAG_base_type':
        return  {
            'kind': 'base',
            'bytes': die.attributes['DW_AT_byte_size'].value,
            'type': die.attributes['DW_AT_name'].value.decode('ascii')
        }
    elif die.tag == 'DW_TAG_typedef':
        return {
            'kind': 'typedef',
#            'name': die_name(die),
            'ref': reference(die)
        }
    elif die.tag == 'DW_TAG_pointer_type':
        return {
            'kind': 'pointer',
            'bytes': die.attributes['DW_AT_byte_size'].value,
            'ref': reference(die)
        }
    elif die.tag in ['DW_TAG_structure_type', 'DW_TAG_union_type']:
        kw = { 'DW_TAG_structure_type': 'struct',
               'DW_TAG_union_type': 'union' }[die.tag]
        return {
            'kind': kw,
            'members': walk_struct_members(die)
        }
    else:
        return { 'kind': 'unknown', 'tag': die.tag }

def inc_offsets00(members, increment):
    def bump(v):
        return {**v, **{'offset': v['offset'] + (increment or 0)}}
    return dict(map(lambda kv: (kv[0], bump(kv[1])), members.items()))

def walk_struct_members(die):
    m = {}
    for child in die.iter_children():
        typ = parse_die(die_type(child))
        attr = child.attributes
        offset = None
        if 'DW_AT_data_member_location' in attr:
            offset = attr['DW_AT_data_member_location'].value
        m[die_name(child)] = {
            'offset': offset,
            **typ
        }
    return m

def record_name(die):
    global names
    name = die_name(die)
    if name == None:
        pdb.set_trace()
    ns = die_namespace(die)
    print("found namespace=%s name=%s" % (ns, name), file=sys.stderr)
    if not name in names[ns]:
        # print("descending ...")
        names[ns][name] = True
        p = parse_die(die)
        names[ns][name] = p

def walk_die_info(die, wanted_names):
    name = die_name(die)
    if name and (name in wanted_names):
        record_name(die)

    for child in die.iter_children():
        walk_die_info(child, wanted_names)

def process_stream(f, wanted_names):
    elffile = ELFFile(f)

    if not elffile.has_dwarf_info():
        print('  file has no DWARF info')
        return

    for CU in elffile.get_dwarf_info().iter_CUs():
        walk_die_info(CU.get_top_DIE(), wanted_names)

def is_archive(filename):
    magic1 = b"!<arch>\n"
    magic2 = b"!<thin>\n"
    with io.open(filename, "rb") as f:
        bytes = f.read(len(magic1))
    return bytes in [magic1, magic2]

def process_archive(filename, wanted_names):
    with arpy.Archive(filename) as ar:
        for header in (ar.infolist()):
            with ar.open(header) as f:
                process_stream(f, wanted_names)
    return names

def process_object(filename, wanted_names):
    with open(filename, "rb") as f:
        process_stream(f, wanted_names)
    return names

if __name__ == '__main__':
    wanted_names = sys.argv[2:]
    filename = sys.argv[1]
    if is_archive(filename):
        ret = process_archive(filename, wanted_names)
    else:
        ret = process_object(filename, wanted_names)

    json.dump(ret, sys.stdout)
