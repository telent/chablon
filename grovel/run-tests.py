import unittest
import pdb
import os

import grovel


def rel(filename):
    return os.path.dirname(os.path.realpath(__file__)) + "/" + filename


class TestCase(unittest.TestCase):
    @classmethod
    def tag(self):
        s =  self.__name__.lstrip("Test")
        return ''.join(['_'+c.lower() if c.isupper() else c for c in s]).lstrip('_')

    @classmethod
    def setUpClass(cls):
        cls.subject = grovel.process_object(rel("testcases.o"), [cls.tag()])

class TestSimpleStruct(TestCase):
    def test_struct_data(self):
        self.assertIn('simple_struct', self.subject['tag'])
        self.assertIn('struct', self.subject['tag'][self.tag()]['kind'])

    def test_members(self):
        members = self.subject['tag'][self.tag()]['members']
        self.assertEqual(0, members['john']['offset'])
        self.assertEqual(4, members['paul']['offset'])
        self.assertEqual(8, members['george']['offset'])
        self.assertEqual(16, members['ringo']['offset'])

class TestAnonNestedStruct(TestCase):
    def test_members(self):
        members = self.subject['tag'][self.tag()]['members']
        expected_members = ['john','len','buf','george','ringo'];
        self.assertEqual([ *members.keys() ], expected_members)
        self.assertEqual(members['len']['offset'], 4)
        self.assertEqual(members['buf']['offset'], 8)
        self.assertEqual(members['george']['offset'], 40)

class TestNestedStruct(TestCase):
    def test_struct_data(self):
        self.assertEqual(self.subject['tag'][self.tag()]['bytes'], 56)

    def test_members(self):
        members = self.subject['tag'][self.tag()]['members']
        expected_members = ['john','nested','george','ringo'];
        self.assertEqual([ *members.keys() ], expected_members)
        self.assertEqual(members['nested']['offset'], 4)
        self.assertEqual(members['nested']['bytes'], 36)
        self.assertEqual(members['george']['offset'], 40)

class TestSelfPointerStruct(TestCase):
    def test_members(self):
        members = self.subject['tag'][self.tag()]['members']
        self.assertIn('next', [ *members.keys() ])

unittest.main()
