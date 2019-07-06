"""Holds all the data necessary to render a menu."""

import Constants
from Entry import Entry
from Entry import ResetTemplateEntry

import copy

class Menu:
    def __init__(self):
        self.properties = {
            'defaultWidth': 0, 'defaultHeight': 0, 'defaultX': 0, 'defaultY': 0, 'defaultColor': 0,
            'defaultTextFlags': 0, 'x': 0, 'y': 0
        }
        self.propertyTokens = {}
        self.exportedVariables = {}
        self.entries = {}
        self.variables = {}
        self.templateEntry = Entry()
        self.gotTemplate = False
        self.name = ''

        self.lastEntry = None
        self.initialX = 0
        self.initialY = 0

        self.onInit = 0
        self.onReturn = 0
        self.onDraw = 0

        for field in Constants.kPreviousEntryFields:
            setattr(self, 'previous' + field.capitalize(), 0)

    def addEntry(self, name, isTemplate):
        entry, internalName = self.createNewEntry(name, isTemplate)

        self.entries[internalName] = entry
        if not isTemplate:
            self.lastEntry = entry

        return entry

    def createNewEntry(self, name, isTemplate):
        if isTemplate:
            return self.createNewTemplateEntry()
        else:
            return self.createNewStandardEntry(name)

    def createNewTemplateEntry(self, isReset=False):
        entry = ResetTemplateEntry() if isReset else Entry()
        entry.template = True
        entry.width = entry.height = 1              # to avoid assert
        entry.ordinal = Constants.kTemplateEntryOrdinalStart + len(self.entries)
        internalName = f'{len(self.entries):02}'    # use a number since it can't be assigned to entry name by the user

        return entry, internalName

    def createNewStandardEntry(self, name):
        assert isinstance(name, str)

        entry = copy.copy(self.templateEntry)

        internalName = name
        if not name:
            internalName = str(len(self.entries))

        entry.name = name
        entry.ordinal = self.getNextEntryOrdinal()

        return entry, internalName

    def getNextEntryOrdinal(self):
        for currentEntry in reversed(list(self.entries.values())):
            if isinstance(currentEntry, Entry) and not currentEntry.template:
                return currentEntry.ordinal + 1
        else:
            return 0
