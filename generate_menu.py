#!/usr/bin/python
import pprint
from dataclasses import dataclass
import argparse


const_str_cache = dict()
def get_str_name(s) :
    if s in const_str_cache :
        return const_str_cache[s];
    else :
        str_name = f'menu_str_{len(const_str_cache)}'
        const_str_cache[s] = str_name
        return str_name

class MenuItem:
    def __init__(self, name, subtype, args, submenu=None) :
        self.name = name
        self.subtype = subtype
        self.str_name = get_str_name(name)
        self.args = args
        self.submenu = submenu
        self.prev_id = -1
        self.next_id = -1

class ChooserItem:
    def __init__(self, name, val) :
        self.name = name
        self.str_name = get_str_name(name)
        self.val = val
        self.prev_id = -1
        self.next_id = -1

output_ch_menu = [
    ('O.MODE', 'CLOCK_UI_CHOOSER', 'CHOOSER_ITEM_OUTPUT_MODE', ['ON', 'OFF', 'AUTO', 'RUNNING', 'END' ]),
    ('O.POLA', 'CLOCK_UI_CHOOSER', 'CHOOSER_ITEM_OUTPUT_POLARITY', ['NORMAL', 'INVERT'])
]

output_menu = [
    ('OP.CH1', output_ch_menu, 'MENU_CONTEXT_CHANNEL', 0),
    ('OP.CH2',  output_ch_menu, 'MENU_CONTEXT_CHANNEL', 1),
    ('OP.CH3',  output_ch_menu, 'MENU_CONTEXT_CHANNEL', 2),
]

input_ch_menu = [
    ('I.POLA', 'CLOCK_UI_CHOOSER', 'CHOOSER_ITEM_INPUT_POLARITY', ['NORMAL', 'INVERT']),
]

input_menu = [
    ('IP.CH1', input_ch_menu, 'MENU_CONTEXT_CHANNEL', 0),
    ('IP.CH2', input_ch_menu, 'MENU_CONTEXT_CHANNEL', 1),
]

timer_menu = [
    ('UP/DWN', 'CLOCK_UI_CHOOSER', 'CHOOSER_ITEM_TIMER_DIRECTION', ['UP', 'DOWN']),
]

display_menu = [
    ('BRIGHT', 'CLOCK_UI_BRIGHTNESS')
]

main_menu = [
    ('OUTPUT',  output_menu),
    ('INPUT',  input_menu),
    ('TIMER', timer_menu),
    ('DISPLAY',  display_menu),
]


chooser_list = list()
chooser_ix_by_objid = dict()

def build_chooser(items) :

    if id(items) in chooser_ix_by_objid :
        return chooser_ix_by_objid[id(items)]

    start_ix = len(chooser_list)

    for value, name_or_tuple in enumerate(items) :
        if type(name_or_tuple) == str :
            name = name_or_tuple
        else :
            name, value = name_or_tuple

        chooser_list.append( ChooserItem(name, value) )

    end_ix = len(chooser_list)

    for k in range(start_ix,end_ix) :
        if k == start_ix :
            chooser_list[k].prev_id = end_ix - 1
        else :
            chooser_list[k].prev_id = k - 1
        if k == end_ix - 1 :
            chooser_list[k].next_id = start_ix
        else :
            chooser_list[k].next_id = k + 1

    chooser_ix_by_objid[id(items)] = start_ix
    return start_ix


# flatten the recursive list of menu items
# store menuid of menu corresponding to list with python objid...
menuid_by_objid = dict()
def flatten_menu(menu, accu=None, parent_index=None):
    if accu is None:
        accu = list()

    start_ix = len(accu)

    # generate entries for each item
    for k, (name, subtype, *args) in enumerate(menu):
        str_name = get_str_name(name)
        # special handling of submenus
        if type(subtype) == list:
            accu.append(MenuItem(name=name, subtype='CLOCK_UI_MENU',
                                 args=args, submenu=subtype))
        else :
            if subtype == 'CLOCK_UI_CHOOSER' :
                args = build_chooser(args[1]), args[0]
            accu.append(MenuItem(name=name, subtype=subtype, args=args))

    # add a "<--" back entry, either to parent menu or main timing display
    if parent_index is not None:
        accu.append(
            MenuItem(name='<--', subtype='CLOCK_UI_MENU', args=(parent_index, )))
    else:
        accu.append(
            MenuItem(name='<--', subtype='CLOCK_UI_TIME_BIG', args=tuple()))

    end_ix = len(accu)

    # generate prev/next ids for the menu items generated so far
    for k in range(start_ix, end_ix):
        m = accu[k]
        if (k == start_ix):
            m.prev_id = end_ix - 1
        else:
            m.prev_id = k - 1

        if (k == end_ix-1):
            m.next_id = start_ix
        else:
            m.next_id = k + 1

    # for all menu-items, fill in the menu IDs and, if they've not
    # yet encountered, recurse function to flatten them, too...
    for k, m in enumerate(accu[start_ix:end_ix-1], start_ix):
        if m.subtype == 'CLOCK_UI_MENU':
#            if id(m.submenu) not in menuid_by_objid:
            this_id = len(accu)
            flatten_menu(m.submenu, accu, k)
#                menuid_by_objid[id(m.submenu)] = this_id
#            else:
#                this_id = menuid_by_objid[id(m.submenu)]
            m.args = (this_id, ) + tuple(m.args)

    return accu


accu = flatten_menu(main_menu)


parser = argparse.ArgumentParser()
parser.add_argument('outputfile')
args = parser.parse_args()


with open(args.outputfile, 'wt') as of:
    print('#include <avr/pgmspace.h>', file=of)
    print('#include "menu_items.h"', file=of)
    print('#include "clock_ui.h"', file=of)
    print('', file=of)

    for s, str_name in const_str_cache.items() :
        print(
            f'static const char {str_name}[] PROGMEM = "{s}";', file=of)

    print('', file=of)

    print('PROGMEM const struct menu_item menu_items[] = {', file=of)
    for k, m in enumerate(accu):
        print(f'\t[{k}]={{ /* {m.name} */', file=of)
        print(
            f'\t\t.name={m.str_name}, .prev_id={m.prev_id}, .next_id={m.next_id},', file=of)

        arg1, arg2, arg3, *argN = tuple(m.args) + (0, 0, 0)

        print(
            f'\t\t.action = {m.subtype}, .arg1={arg1}, .arg2={arg2}, .arg3={arg3}', file=of)

        print(f'\t}},', file=of)

    print('};', file=of)

    print('', file=of)

    print('PROGMEM const struct chooser_item chooser_items[] = {', file=of)
    for k, m in enumerate(chooser_list):
        print(f'\t[{k}]={{ /* {m.name} */', file=of)
        print(
            f'\t\t.name={m.str_name}, .prev_id={m.prev_id}, .next_id={m.next_id}, .value={m.val}', file=of)
        print(f'\t}},', file=of)

    print('};', file=of)
