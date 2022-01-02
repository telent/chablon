import unittest
import pdb
import os

import grovel


def rel(filename):
    return os.path.dirname(os.path.realpath(__file__)) + "/" + filename

class TestStruct(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.subject = grovel.process_object(rel("testcases.o"), ["test1"])

    def test_struct_data(self):
        self.assertIn('test1', self.subject['tag'])
        self.assertIn('struct', self.subject['tag']['test1']['kind'])

    def test_members(self):
        members = self.subject['tag']['test1']['members']
        self.assertEqual(0, members['john']['offset'])
        self.assertEqual(4, members['paul']['offset'])
        self.assertEqual(8, members['george']['offset'])
        self.assertEqual(16, members['ringo']['offset'])

class TestAnonNestedStruct(unittest.TestCase):
    TAG = "anon_nested_struct"
    @classmethod
    def setUpClass(cls):
        cls.subject = grovel.process_object(rel("testcases.o"), [cls.TAG])

    def test_members(self):
        members = self.subject['tag'][self.TAG]['members']
        expected_members = ['john','len','buf','george','ringo'];
        self.assertEqual([ *members.keys() ], expected_members)
        self.assertEqual(members['len']['offset'], 4)
        self.assertEqual(members['buf']['offset'], 8)
        self.assertEqual(members['george']['offset'], 40)


unittest.main()
