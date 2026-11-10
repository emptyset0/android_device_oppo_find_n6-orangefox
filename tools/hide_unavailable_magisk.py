"""Remove unbundled Magisk add-on choices; keep the native installed-module manager."""
import re
import xml.etree.ElementTree as ET


def patch_advanced(blob):
    text = blob.decode('utf-8')
    managers = list(re.finditer(r'(?ms)^[ \t]*<listitem name="Magisk Manager">\n.*?^[ \t]*</listitem>\n', text))
    if len(managers) == 2:
        real, fallback = managers
        assert 'var2="%magisk_path%"' in real[0] and 'op="!=" var2="%magisk_path%"' in fallback[0]
        assert '%ap_path%' in fallback[0] and '%ksu_path%' in fallback[0]
        text = text[:fallback.start()] + text[fallback.end():]
        text = text.replace('<condition var1="fileexists" var2="%magisk_path%"/>',
                            '<condition var1="fileexists" var2="%magisk_path%/util_functions.sh"/>\n'
                            '\t\t\t\t\t<condition var1="fileexists" var2="%rmm_path%"/>', 1)
        page = text.index('<page name="fox_modules">')
        title = text.index('<text>Magisk</text>', page)
        start = text.rindex('\n\t\t\t<text style="caption">', page, title)
        end = text.index('</listbox>', title) + len('</listbox>')
        block = text[start:end]
        assert block.count('<listitem ') == 2 and '>mod_magisk</action>' in block and '>mod_unmagisk</action>' in block
        text = text[:start] + text[end:]
        # Move the next native group up so the absent addon leaves no gap.
        other = text.index('<text>{@cust_oth}</text>', start)
        listbox = text.index('<listbox style="group_list">', other)
        placement_end = text.index('/>', listbox) + 2
        region = text[start:placement_end]
        assert region.count('%row3_1a_y%') == region.count('%row3_2a_y%') == 1
        text = text[:start] + region.replace('%row3_1a_y%', '%row1_1_y%').replace('%row3_2a_y%', '%row1_2_y%') + text[placement_end:]
    else:
        assert len(managers) == 1

    xml = ET.fromstring(text)
    advanced = xml.find('.//page[@name="advanced"]')
    addons = xml.find('.//page[@name="fox_modules"]')
    assert advanced is not None and addons is not None
    real = advanced.findall('.//listitem[@name="Magisk Manager"]')
    assert len(real) == 1
    assert {c.get('var2') for c in real[0].findall('condition') if c.get('var1') == 'fileexists'} == {
        '%magisk_path%/util_functions.sh', '%rmm_path%'}
    assert all('Magisk' not in (item.get('name') or '') for item in addons.iter('listitem'))
    assert not any(child.text == 'Magisk' for child in addons.iter('text'))
    assert advanced.find('.//listitem[@name="{@reflash_twrp}"]') is not None
    assert advanced.find('.//listitem[@name="{@ksu_modules}"]') is not None
    assert addons.find('.//listitem[@name="{@module_install} KernelSU %ksu_ver%"]') is not None
    return text.encode('utf-8')
