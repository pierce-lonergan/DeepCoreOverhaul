#!/usr/bin/env python3
"""
Self-tests for addrlint.

The linter is the project's only automated safety net against corrupting the
original executable's memory. A safety net nobody tested is not a safety net, so
these cases pin the behaviour that matters: it must FIND real overlaps, must NOT
invent false ones, must ignore commented-out declarations, and must refuse to
guess at sizes it does not know.

Run:  python tools/addrlint/test_addrlint.py
"""

import os
import shutil
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import addrlint  # noqa: E402


class Tree:
    """Throwaway source tree so tests never touch the real repo."""

    def __init__(self):
        self.root = tempfile.mkdtemp(prefix='addrlint-test-')
        os.makedirs(os.path.join(self.root, 'src', 'openlrr'))

    def write(self, relpath, text):
        path = os.path.join(self.root, 'src', 'openlrr', relpath)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w', encoding='utf-8') as fh:
            fh.write(text)

    def analyse(self):
        sizes, origin, bindings = addrlint.scan(self.root)
        bindings = addrlint.resolve(bindings, sizes)
        overlaps, known = addrlint.find_overlaps(bindings)
        gaps = addrlint.compute_gaps(known)
        return {
            'sizes': sizes, 'bindings': bindings, 'overlaps': overlaps,
            'known': known, 'gaps': gaps,
            'unknown': [b for b in bindings if not b['size']],
        }

    def close(self):
        shutil.rmtree(self.root, ignore_errors=True)


class AddrLintTest(unittest.TestCase):

    def setUp(self):
        self.t = Tree()
        self.addCleanup(self.t.close)

    # -- detection ---------------------------------------------------------

    def test_detects_real_overlap(self):
        """
        A struct grown past its neighbour must be caught.

        Here Next_Globs [0x500080, 0x500090) falls entirely INSIDE
        Big_Globs [0x500000, 0x500100), so the overlap is the whole of
        Next_Globs -- 0x10 bytes -- not the distance from its start to
        Big_Globs' end. Getting this wrong under-reports containment.
        """
        self.t.write('a.h', 'assert_sizeof(Big_Globs, 0x100);\nassert_sizeof(Next_Globs, 0x10);\n')
        self.t.write('a.cpp',
                     'NS::Big_Globs & NS::bigGlobs = *(NS::Big_Globs*)0x00500000;\n'
                     'NS::Next_Globs & NS::nextGlobs = *(NS::Next_Globs*)0x00500080;\n')
        r = self.t.analyse()
        self.assertEqual(len(r['overlaps']), 1)
        self.assertEqual(r['overlaps'][0]['bytes'], 0x10)

    def test_partial_overlap_measures_the_intersection(self):
        """Tail-into-head overlap: the intersection, not either full size."""
        self.t.write('a.h', 'assert_sizeof(A_Globs, 0x100);\nassert_sizeof(B_Globs, 0x100);\n')
        self.t.write('a.cpp',
                     'NS::A_Globs & NS::a = *(NS::A_Globs*)0x00500000;\n'
                     'NS::B_Globs & NS::b = *(NS::B_Globs*)0x00500080;\n')
        r = self.t.analyse()
        self.assertEqual(len(r['overlaps']), 1)
        self.assertEqual(r['overlaps'][0]['bytes'], 0x80)

    def test_adjacent_is_not_overlap(self):
        """Exactly touching regions are legal and extremely common."""
        self.t.write('a.h', 'assert_sizeof(A_Globs, 0x100);\nassert_sizeof(B_Globs, 0x10);\n')
        self.t.write('a.cpp',
                     'NS::A_Globs & NS::a = *(NS::A_Globs*)0x00500000;\n'
                     'NS::B_Globs & NS::b = *(NS::B_Globs*)0x00500100;\n')
        r = self.t.analyse()
        self.assertEqual(r['overlaps'], [])
        self.assertEqual(r['gaps'][0]['slack'], 0)

    def test_slack_is_measured(self):
        self.t.write('a.h', 'assert_sizeof(A_Globs, 0x10);\nassert_sizeof(B_Globs, 0x10);\n')
        self.t.write('a.cpp',
                     'NS::A_Globs & NS::a = *(NS::A_Globs*)0x00500000;\n'
                     'NS::B_Globs & NS::b = *(NS::B_Globs*)0x00500018;\n')
        r = self.t.analyse()
        self.assertEqual(r['gaps'][0]['slack'], 8)

    def test_three_way_overlap_reports_each_pair(self):
        """A big region swallowing two others must report both collisions."""
        self.t.write('a.h', 'assert_sizeof(Huge, 0x100);\n')
        self.t.write('a.cpp',
                     'NS::Huge & NS::h = *(NS::Huge*)0x00500000;\n'
                     'sint32 & NS::x = *(sint32*)0x00500010;\n'
                     'sint32 & NS::y = *(sint32*)0x00500020;\n')
        r = self.t.analyse()
        self.assertEqual(len(r['overlaps']), 2)

    # -- the real regression: a scalar hiding inside a gap ------------------

    def test_scalar_inside_gap_is_counted(self):
        """
        Regression for a real mistake. statsGlobs ends at 0x00504188 and textGlobs
        begins at 0x00504190, which looks like 8 free bytes -- but a live 4-byte
        variable sits at 0x00504188. The linter must surface that variable as its
        own region so the true free slack reads as 4, not 8.
        """
        self.t.write('a.h', 'assert_sizeof(Stats_Globs, 0x5b0);\nassert_sizeof(Text_Globs, 0x4dc);\n')
        self.t.write('a.cpp',
                     'LegoRR::Stats_Globs & LegoRR::statsGlobs = *(LegoRR::Stats_Globs*)0x00503bd8;\n'
                     'bool32 & LegoRR::g_Teleporter_BOOL_00504188 = *(bool32*)0x00504188;\n'
                     'LegoRR::Text_Globs& LegoRR::textGlobs = *(LegoRR::Text_Globs*)0x00504190;\n')
        r = self.t.analyse()
        self.assertEqual(r['overlaps'], [], 'real layout must be overlap-free')
        names = [b['name'] for b in r['known']]
        self.assertIn('LegoRR::g_Teleporter_BOOL_00504188', names)
        slack_after_stats = [g['slack'] for g in r['gaps'] if 'statsGlobs' in g['after']['name']]
        self.assertEqual(slack_after_stats, [0], 'the scalar sits flush against statsGlobs')
        slack_after_bool = [g['slack'] for g in r['gaps']
                            if 'Teleporter' in g['after']['name']]
        self.assertEqual(slack_after_bool, [4], 'only 4 bytes are genuinely free')

    # -- parsing hygiene ---------------------------------------------------

    def test_commented_bindings_ignored(self):
        self.t.write('a.h', 'assert_sizeof(A_Globs, 0x100);\n')
        self.t.write('a.cpp',
                     '//NS::A_Globs & NS::a = *(NS::A_Globs*)0x00500000;\n'
                     '  //NS::A_Globs & NS::b = *(NS::A_Globs*)0x00500000;\n')
        self.assertEqual(self.t.analyse()['bindings'], [])

    def test_commented_sizeof_ignored(self):
        self.t.write('a.h', '//assert_sizeof(A_Globs, 0x100);\n')
        self.t.write('a.cpp', 'NS::A_Globs & NS::a = *(NS::A_Globs*)0x00500000;\n')
        r = self.t.analyse()
        self.assertEqual(len(r['unknown']), 1, 'size must not be inferred from a commented assert')

    def test_unknown_type_is_flagged_not_guessed(self):
        """Refusing to guess is the whole point -- a wrong size is worse than a gap."""
        self.t.write('a.cpp', 'NS::Mystery & NS::m = *(NS::Mystery*)0x00500000;\n')
        r = self.t.analyse()
        self.assertEqual(len(r['unknown']), 1)
        self.assertEqual(r['unknown'][0]['size_src'], 'UNKNOWN')
        self.assertEqual(r['overlaps'], [], 'unsized regions cannot produce overlap claims')

    def test_primitive_sizes_used(self):
        self.t.write('a.cpp',
                     'sint32 & NS::a = *(sint32*)0x00500000;\n'
                     'Point2F & NS::b = *(Point2F*)0x00500004;\n')
        r = self.t.analyse()
        self.assertEqual([b['size'] for b in r['known']], [4, 8])

    def test_namespace_is_stripped_for_size_lookup(self):
        """Binding casts are namespace-qualified; assert_sizeof is not."""
        self.t.write('a.h', 'assert_sizeof(Water_Globs, 0x29ec);\n')
        self.t.write('a.cpp',
                     'LegoRR::Water_Globs & LegoRR::waterGlobs = *(LegoRR::Water_Globs*)0x0054a520;\n')
        r = self.t.analyse()
        self.assertEqual(len(r['known']), 1)
        self.assertEqual(r['known'][0]['size'], 0x29ec)
        self.assertEqual(r['known'][0]['size_src'], 'assert_sizeof')

    def test_decimal_sizeof_accepted(self):
        self.t.write('a.h', 'assert_sizeof(A, 16);\n')
        self.t.write('a.cpp', 'NS::A & NS::a = *(NS::A*)0x00500000;\n')
        self.assertEqual(self.t.analyse()['known'][0]['size'], 16)


if __name__ == '__main__':
    unittest.main(verbosity=2)
