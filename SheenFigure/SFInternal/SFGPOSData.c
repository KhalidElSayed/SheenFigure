/*
 * Copyright (C) 2012 SheenFigure
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>

#include "SFTypes.h"
#include "SFInternal.h"
#include "SFGPOSData.h"

void **gposTable;

static DeviceTable nullDevice() {
    DeviceTable device;
    device.startSize = 0;
    device.endSize = 0;
    device.deltaFormat = 0;
    device.deltaValue = 0;
    
    return device;
}

static void SFReadValueRecord(const SFUByte * const table, SFUShort *offset, ValueRecord *tablePtr, ValueFormat format) {
    if (format & vfXPlacement) {
        tablePtr->xPlacement = SFReadUShort(table, *offset);
        *offset += 2;
        
#ifdef LOOKUP_TEST
        printf("\n          X Placement: %d", tablePtr->xPlacement);
#endif
    } else
        tablePtr->xPlacement = 0;
    
    if (format & vfYPlacement) {
        tablePtr->yPlacement = SFReadUShort(table, *offset);
        *offset += 2;
        
#ifdef LOOKUP_TEST
        printf("\n          Y Placement: %d", tablePtr->yPlacement);
#endif
    } else
        tablePtr->yPlacement = 0;
    
    if (format & vfXAdvance) {
        tablePtr->xAdvance = SFReadUShort(table, *offset);
        *offset += 2;
        
#ifdef LOOKUP_TEST
        printf("\n          X Advance: %d", tablePtr->xAdvance);
#endif
    } else
        tablePtr->xAdvance = 0;
    
    if (format & vfYAdvance) {
        tablePtr->yAdvance = SFReadUShort(table, *offset);
        *offset += 2;
        
#ifdef LOOKUP_TEST
        printf("\n          Y Advance: %d", tablePtr->yAdvance);
#endif
    } else
        tablePtr->yAdvance = 0;
    
    DeviceTable nullDev = nullDevice();
    tablePtr->xPlaDevice = nullDev;
    tablePtr->yPlaDevice = nullDev;
    tablePtr->xAdvDevice = nullDev;
    tablePtr->yAdvDevice = nullDev;
    
    if (format & vfXPlaDevice) {
        SFUShort xPlaDeviceOffset = SFReadUShort(table, *offset);
        
#ifdef LOOKUP_TEST
        printf("\n          X Placement Device Table:");
        printf("\n           Offset: %d", xPlaDeviceOffset);
#endif

        if (xPlaDeviceOffset)
            SFReadDeviceTable(gposTable[xPlaDeviceOffset], &tablePtr->xPlaDevice);
            
        *offset += 2;
    }
    
    if (format & vfYPlaDevice) {
        SFUShort yPlaDeviceOffset = SFReadUShort(table, *offset);
        
#ifdef LOOKUP_TEST
        printf("\n          Y Placement Device Table:");
        printf("\n           Offset: %d", yPlaDeviceOffset);
#endif
        
        if (yPlaDeviceOffset)
            SFReadDeviceTable(gposTable[yPlaDeviceOffset], &tablePtr->yPlaDevice);
        
        *offset += 2;
    }
    
    if (format & vfXAdvDevice) {
        SFUShort xAdvDeviceOffset = SFReadUShort(table, *offset);
        
#ifdef LOOKUP_TEST
        printf("\n          X Advance Device Table:");
        printf("\n           Offset: %d", xAdvDeviceOffset);
#endif
        
        if (xAdvDeviceOffset)
            SFReadDeviceTable(gposTable[xAdvDeviceOffset], &tablePtr->xAdvDevice);
        
        *offset += 2;
    }
    
    if (format & vfYAdvDevice) {
        SFUShort yAdvDeviceOffset = SFReadUShort(table, *offset);
        
#ifdef LOOKUP_TEST
        printf("\n          Y Advance Device Table:");
        printf("\n           Offset: %d", yAdvDeviceOffset);
#endif
        
        if (yAdvDeviceOffset)
            SFReadDeviceTable(gposTable[yAdvDeviceOffset], &tablePtr->yAdvDevice);
        
        *offset += 2;
    }
}


#ifdef GPOS_SINGLE

static void SFReadSingleAdjustment(const SFUByte * const saTable, SingleAdjustmentPosSubtable *tablePtr) {
    SFUShort posFormat = SFReadUShort(saTable, 0);
    tablePtr->posFormat = posFormat;
    
    SFUShort coverageOffset = SFReadUShort(saTable, 2);
    
#ifdef LOOKUP_TEST
    printf("\n      Single Adjustment Positioning Table:");
    printf("\n       Positioning Format: %d", posFormat);
    printf("\n       Coverage Table:");
    printf("\n        Offset: %d", coverageOffset);
#endif
    
    SFReadCoverageTable(&saTable[coverageOffset], &tablePtr->coverage);
    tablePtr->valueFormat = SFReadUShort(saTable, 4);
    
#ifdef LOOKUP_TEST
    printf("\n       Value Format: %d", tablePtr->valueFormat);
#endif

    switch (posFormat) {

#ifdef GPOS_SINGLE_FORMAT1
        case 1:
        {
            SFUShort valueOffset = 6;
            
#ifdef LOOKUP_TEST
            printf("\n       Value Record:");
#endif
            
            SFReadValueRecord(saTable, &valueOffset, &tablePtr->format.format1.value, tablePtr->valueFormat);
        }
            break;
#endif
            
#ifdef GPOS_SINGLE_FORMAT2
        case 2:
        {
            SFUShort valueCount = SFReadUShort(saTable, 6);
            tablePtr->format.format2.valueCount = valueCount;
            
#ifdef LOOKUP_TEST
            printf("\n       Total Values: %d", valueCount);
#endif
            
            ValueRecord *values = malloc(sizeof(ValueRecord) * valueCount);
            
            SFUShort nextValueOffset = 8;            
            for (SFUShort i = 0; i < valueCount; i++) {
#ifdef LOOKUP_TEST
                printf("\n       Value Record At Index %d:", i);
#endif

                SFReadValueRecord(saTable, &nextValueOffset, &values[i], tablePtr->valueFormat);
            }
            
            tablePtr->format.format2.value = values;
        }
            break;
#endif
    }
}

static void SFFreeSingleAdjustment(SingleAdjustmentPosSubtable *tablePtr) {
    SFFreeCoverageTable(&tablePtr->coverage);
    
#ifdef GPOS_SINGLE_FORMAT2
    if (tablePtr->posFormat == 2)
        free(tablePtr->format.format2.value);
#endif
}

#endif


#ifdef GPOS_PAIR

static void SFReadPairAdjustment(const SFUByte * const paTable, PairAdjustmentPosSubtable *tablePtr) {
    SFUShort posFormat = SFReadUShort(paTable, 0);
    tablePtr->posFormat = posFormat;
    
    SFUShort coverageOffset = SFReadUShort(paTable, 2);
    
#ifdef LOOKUP_TEST
    printf("\n      Pair Adjustment Positioning Table:");
    printf("\n       Positioning Format: %d", posFormat);
    printf("\n       Coverage Table:");
    printf("\n        Offset: %d", coverageOffset);
#endif
    
    SFReadCoverageTable(&paTable[coverageOffset], &tablePtr->coverage);
    
    tablePtr->valueFormat1 = SFReadUShort(paTable, 4);
    tablePtr->valueFormat2 = SFReadUShort(paTable, 6);
    
#ifdef LOOKUP_TEST
    printf("\n       Value Format 1: %d", tablePtr->valueFormat1);
    printf("\n       Value Format 2: %d", tablePtr->valueFormat2);
#endif
    
    switch (posFormat) {
            
#ifdef GPOS_PAIR_FORMAT1
        case 1:
        {
            SFUShort pairSetCount = SFReadUShort(paTable, 8);
            tablePtr->format.format1.pairSetCount = pairSetCount;

#ifdef LOOKUP_TEST
            printf("\n       Total Pair Sets: %d", pairSetCount);
#endif
            
            PairSetTable *pairSetTables = malloc(sizeof(PairSetTable) * pairSetCount);
            
            for (SFUShort i = 0; i < pairSetCount; i++) {
                SFUShort pairSetOffset = SFReadUShort(paTable, 10);
                const SFUByte * const psTable = &paTable[pairSetOffset];
                
#ifdef LOOKUP_TEST
                printf("\n       Pair Set At Index %d:", i);
                printf("\n        Offset: %d", pairSetOffset);
#endif
                
                SFUShort pairValueCount = SFReadUShort(psTable, 0);
                pairSetTables[i].pairValueCount = pairValueCount;
                
#ifdef LOOKUP_TEST
                printf("\n        Total Pair Values: %d", pairValueCount);
#endif
                
                PairValueRecord *pairValueRecords = malloc(sizeof(PairValueRecord) * pairValueCount);
                
                SFUShort beginOffset = 2;
                for (SFUShort j = 0; j < pairValueCount; j++) {
                    pairValueRecords[j].secondGlyph = SFReadUShort(psTable, beginOffset);
                    beginOffset += 2;
                    
#ifdef LOOKUP_TEST
                    printf("\n        Pair Value At Index: %d", j);
                    printf("\n         Second Glyph: %d", pairValueRecords[j].secondGlyph);
                    printf("\n         Value 1 Record:");
#endif
                    
                    SFReadValueRecord(psTable, &beginOffset, &pairValueRecords[j].value1, tablePtr->valueFormat1);
                    
#ifdef LOOKUP_TEST
                    printf("\n         Value 2 Record:");
#endif
                    
                    SFReadValueRecord(psTable, &beginOffset, &pairValueRecords[j].value2, tablePtr->valueFormat2);
                }
                
                pairSetTables[i].pairValueRecord = pairValueRecords;
            }
            
            tablePtr->format.format1.pairSetTable = pairSetTables;
        }
            break;
#endif
            
#ifdef GPOS_PAIR_FORMAT2
        case 2:
        {
            SFUShort classDef1Offset = SFReadUShort(paTable, 8);
            
#ifdef LOOKUP_TEST
            printf("\n       Class Definition 1:");
            printf("\n        Offset: %d", classDef1Offset);
#endif
            
            SFReadClassDefTable(&paTable[classDef1Offset], &tablePtr->format.format2.classDef1);
            
            SFUShort classDef2Offset = SFReadUShort(paTable, 10);
            
#ifdef LOOKUP_TEST
            printf("\n       Class Definition 2:");
            printf("\n        Offset: %d", classDef2Offset);
#endif
            
            SFReadClassDefTable(&paTable[classDef2Offset], &tablePtr->format.format2.classDef2);

            SFUShort class1Count = SFReadUShort(paTable, 12);
            SFUShort class2Count = SFReadUShort(paTable, 14);
            
#ifdef LOOKUP_TEST
            printf("\n       Total Classes in Class Definition 1: %d", class1Count);
            printf("\n       Total Classes in Class Definition 2: %d", class2Count);
#endif
            
            tablePtr->format.format2.class1Count = class1Count;
            tablePtr->format.format2.class2Count = class2Count;
            
            Class1Record *class1Records = malloc(sizeof(Class1Record) * class1Count);
            
            SFUShort beginOffset = 16;
            for (SFUShort i = 0; i < class1Count; i++) {
#ifdef LOOKUP_TEST
                printf("\n       Class 1 Record At Index %d:", i);
#endif
                
                Class2Record *class2Records = malloc(sizeof(Class2Record) * class2Count);

                for (SFUShort j = 0; j < class2Count; j++) {
#ifdef LOOKUP_TEST
                    printf("\n        Class 2 Record At Index %d:", j);
                    printf("\n         Value 1 Record:");
#endif
                    
                    SFReadValueRecord(paTable, &beginOffset, &class2Records[j].value1, tablePtr->valueFormat1);
                    
#ifdef LOOKUP_TEST
                    printf("\n         Value 2 Record:");
#endif
                    
                    SFReadValueRecord(paTable, &beginOffset, &class2Records[j].value2, tablePtr->valueFormat2);
                }
                
                class1Records[i].class2Record = class2Records;
            }
            
            tablePtr->format.format2.class1Record = class1Records;
        }
            break;
#endif
    }
}

static void SFFreePairAdjustment(PairAdjustmentPosSubtable *tablePtr) {
    SFFreeCoverageTable(&tablePtr->coverage);
    
    switch (tablePtr->posFormat) {
            
#ifdef GPOS_PAIR_FORMAT1
        case 1:
            for (SFUShort i = 0; i < tablePtr->format.format1.pairSetCount; i++)
                free(tablePtr->format.format1.pairSetTable[i].pairValueRecord);

            free(tablePtr->format.format1.pairSetTable);
            
            break;
#endif
            
#ifdef GPOS_PAIR_FORMAT2
        case 2:
            SFFreeClassDefTable(&tablePtr->format.format2.classDef1);
            SFFreeClassDefTable(&tablePtr->format.format2.classDef2);
            
            for (SFUShort i = 0; i < tablePtr->format.format2.class1Count; i++)
                free(tablePtr->format.format2.class1Record[i].class2Record);

            free(tablePtr->format.format2.class1Record);
            
            break;
#endif
    }
}

#endif


#ifdef GPOS_ANCHOR

static void SFReadAnchorTable(const SFUByte * const aTable, AnchorTable *tablePtr) {
    tablePtr->anchorFormat = SFReadUShort(aTable, 0);
    tablePtr->xCoordinate = SFReadUShort(aTable, 2);
    tablePtr->yCoordinate = SFReadUShort(aTable, 4);
    
#ifdef LOOKUP_TEST
    printf("\n         Anchor Format: %d", tablePtr->anchorFormat);
    printf("\n         X Coordinate: %d", tablePtr->xCoordinate);
    printf("\n         Y Coordinate: %d", tablePtr->yCoordinate);
#endif
    
    switch (tablePtr->anchorFormat) {
        case 2:
            tablePtr->format.format2.anchorPoint = SFReadUShort(aTable, 6);
            
#ifdef LOOKUP_TEST
            printf("\n         Anchor Point: %d", tablePtr->format.format2.anchorPoint);
#endif
            break;
            
        case 3:
        {
            SFUShort xDeviceOffset = SFReadUShort(aTable, 6);
            
#ifdef LOOKUP_TEST
            printf("\n         X Device Table:");
            printf("\n          Offset: %d", xDeviceOffset);
#endif
            SFReadDeviceTable(&aTable[xDeviceOffset], &tablePtr->format.format3.xDeviceTable);
            
            SFUShort yDeviceOffset = SFReadUShort(aTable, 8);
            
#ifdef LOOKUP_TEST
            printf("\n         Y Device Table:");
            printf("\n          Offset: %d", yDeviceOffset);
#endif
            SFReadDeviceTable(&aTable[yDeviceOffset], &tablePtr->format.format3.yDeviceTable);
        }
            break;
    }
}

#endif


#ifdef GPOS_CURSIVE

static void SFReadCursiveAttachment(const SFUByte * const caTable, CursiveAttachmentPosSubtable *tablePtr) {
    SFUShort posFormat = SFReadUShort(caTable, 0);
    tablePtr->posFormat = posFormat;
    
    SFUShort coverageOffset = SFReadUShort(caTable, 2);
    
#ifdef LOOKUP_TEST
    printf("\n      Cursive Attachment Positioning Table:");
    printf("\n       Positioning Format: %d", posFormat);
    printf("\n       Coverage Table:");
    printf("\n        Offset: %d", coverageOffset);
#endif
    
    SFReadCoverageTable(&caTable[coverageOffset], &tablePtr->coverage);
    
    SFUShort entryExitCount = SFReadUShort(caTable, 4);
    
#ifdef LOOKUP_TEST
    printf("\n       Total Entry Exits: %d", entryExitCount);
#endif
    
    EntryExitRecord *entryExitRecords = malloc(sizeof(EntryExitRecord) * entryExitCount);
    
    SFUShort beginOffset = 6;
    for (SFUShort i = 0; i < entryExitCount; i++) {
#ifdef LOOKUP_TEST
        printf("\n       Entry Exit At Index: %d", i);
#endif
        
        SFUShort entryAnchorOffset = SFReadUShort(caTable, beginOffset);
        beginOffset += 2;
        
#ifdef LOOKUP_TEST
        printf("\n        Entry Anchor:");
        printf("\n         Offset: %d", entryAnchorOffset);
#endif

        entryExitRecords[i].hasEntryAnchor = entryAnchorOffset;
        SFReadAnchorTable(&caTable[entryAnchorOffset], &entryExitRecords[i].entryAnchor);
        
        SFUShort exitAnchorOffset = SFReadUShort(caTable, beginOffset);
        beginOffset += 2;
        
#ifdef LOOKUP_TEST
        printf("\n        Exit Anchor:");
        printf("\n         Offset: %d", exitAnchorOffset);
#endif
        
        entryExitRecords[i].hasExitAnchor = exitAnchorOffset;
        SFReadAnchorTable(&caTable[exitAnchorOffset], &entryExitRecords[i].exitAnchor);
    }
    
    tablePtr->entryExitRecord = entryExitRecords;
}

static void SFFreeCursiveAttachment(CursiveAttachmentPosSubtable *tablePtr) {
    SFFreeCoverageTable(&tablePtr->coverage);
    free(tablePtr->entryExitRecord);
}

#endif


#ifdef GPOS_MARK

static void SFReadMarkArray(const SFUByte * const maTable, MarkArrayTable *tablePtr)  {
    SFUShort markCount = SFReadUShort(maTable, 0);
    tablePtr->markCount = markCount;
    
#ifdef LOOKUP_TEST
    printf("\n        Total Mark Records: %d", markCount);
#endif
    
    MarkRecord *markRecords = malloc(sizeof(MarkRecord) * markCount);
    
    SFUShort beginOffset = 2;
    for (SFUShort i = 0; i < markCount; i++) {
        markRecords[i].cls = SFReadUShort(maTable, beginOffset);
        
#ifdef LOOKUP_TEST
        printf("\n        Mark Record At Index %d:", i);
        printf("\n         Class: %d", markRecords[i].cls);
#endif
        
        beginOffset += 2;
        SFUShort markAnchorOffset = SFReadUShort(maTable, beginOffset);
        
#ifdef LOOKUP_TEST
        printf("\n         Mark Anchor Table:");
        printf("\n          Offset: %d", markAnchorOffset);
#endif
    
        SFReadAnchorTable(&maTable[markAnchorOffset], &markRecords[i].markAnchor);
        
        beginOffset += 2;
    }
    
    tablePtr->markRecord = markRecords;
}

static void SFFreeMarkArray(MarkArrayTable *tablePtr) {
    free(tablePtr->markRecord);
}

#endif


#ifdef GPOS_MARK_TO_BASE

static void SFReadMarkToBaseAttachment(const SFUByte * const mtbTable, MarkToBaseAttachmentPosSubtable *tablePtr) {
    SFUShort posFormat = SFReadUShort(mtbTable, 0);
    tablePtr->posFormat = posFormat;
    
    SFUShort markCoverageOffset = SFReadUShort(mtbTable, 2);
    
#ifdef LOOKUP_TEST
    printf("\n      Mark To Base Attachment Positioning Table:");
    printf("\n       Positioning Format: %d", posFormat);
    printf("\n       Mark Coverage Table:");
    printf("\n        Offset: %d", markCoverageOffset);
#endif
    
    SFReadCoverageTable(&mtbTable[markCoverageOffset], &tablePtr->markCoverage);
    
    SFUShort baseCoverageOffset = SFReadUShort(mtbTable, 4);
    
#ifdef LOOKUP_TEST
    printf("\n       Base Coverage Table:");
    printf("\n        Offset: %d", baseCoverageOffset);
#endif
    
    SFReadCoverageTable(&mtbTable[baseCoverageOffset], &tablePtr->baseCoverage);
    
    SFUShort classCount = SFReadUShort(mtbTable, 6);
    tablePtr->classCount = classCount;
    
#ifdef LOOKUP_TEST
    printf("\n       Total Classes: %d", classCount);
#endif
    
    SFUShort markArrayOffset = SFReadUShort(mtbTable, 8);
    
#ifdef LOOKUP_TEST
    printf("\n       Mark Array Table:");
    printf("\n        Offset: %d", markArrayOffset);
#endif
    
    SFReadMarkArray(&mtbTable[markArrayOffset], &tablePtr->markArray);
    
    SFUShort baseArrayOffset = SFReadUShort(mtbTable, 10);
    
#ifdef LOOKUP_TEST
    printf("\n       Base Array Table:");
    printf("\n        Offset: %d", baseArrayOffset);
#endif
    
    const SFUByte * const baTable = &mtbTable[baseArrayOffset];
    
    BaseArrayTable baseArrayTable;
    
    SFUShort baseCount = SFReadUShort(baTable, 0);
    baseArrayTable.baseCount = baseCount;
    
#ifdef LOOKUP_TEST
    printf("\n        Total Base Records: %d", baseCount);
#endif
    
    BaseRecord *baseRecords = malloc(sizeof(BaseRecord) * baseCount);
    
    SFUShort beginOffset = 2;
    for (SFUShort i = 0; i < baseCount; i++) {
#ifdef LOOKUP_TEST
        printf("\n        Base Record At Index %d:", i);
        printf("\n         Total Base Anchors: %d", classCount);
#endif
        
        AnchorTable *baseAnchors = malloc(sizeof(AnchorTable) * classCount);
        
        SFUShort j = 0;
        for (; j < classCount; j++) {
            SFUShort baseAnchorOffset = SFReadUShort(baTable, beginOffset);
            
#ifdef LOOKUP_TEST
            printf("\n         Base Anchor At Index %d:", j);
            printf("\n          Offset: %d", baseAnchorOffset);
#endif
            
            SFReadAnchorTable(&baTable[baseAnchorOffset], &baseAnchors[j]);
            
            beginOffset += 2;
        }

        baseRecords[i].baseAnchor = baseAnchors;
    }
    
    baseArrayTable.baseRecord = baseRecords;
    tablePtr->baseArray = baseArrayTable;
}

static void SFFreeMarkToBaseAttachment(MarkToBaseAttachmentPosSubtable *tablePtr) {
    SFFreeCoverageTable(&tablePtr->markCoverage);
    SFFreeCoverageTable(&tablePtr->baseCoverage);
    SFFreeMarkArray(&tablePtr->markArray);

    for (SFUShort i = 0; i < tablePtr->baseArray.baseCount; i++)
        free(tablePtr->baseArray.baseRecord[i].baseAnchor);

    free(tablePtr->baseArray.baseRecord);
}

#endif


#ifdef GPOS_MARK_TO_LIGATURE

static void SFReadMarkToLigatureAttachment(const SFUByte * const mtlTable, MarkToLigatureAttachmentPosSubtable *tablePtr) {
    SFUShort posFormat = SFReadUShort(mtlTable, 0);
    tablePtr->posFormat = posFormat;
    
    SFUShort markCoverageOffset = SFReadUShort(mtlTable, 2);
    
#ifdef LOOKUP_TEST
    printf("\n      Mark To Ligature Attachment Positioning Table:");
    printf("\n       Positioning Format: %d", posFormat);
    printf("\n       Mark Coverage Table:");
    printf("\n        Offset: %d", markCoverageOffset);
#endif
    
    SFReadCoverageTable(&mtlTable[markCoverageOffset], &tablePtr->markCoverage);
    
    SFUShort ligatureCoverageOffset = SFReadUShort(mtlTable, 4);
    
#ifdef LOOKUP_TEST
    printf("\n       Ligature Coverage Table:");
    printf("\n        Offset: %d", ligatureCoverageOffset);
#endif
    
    SFReadCoverageTable(&mtlTable[ligatureCoverageOffset], &tablePtr->ligatureCoverage);
    
    SFUShort classCount = SFReadUShort(mtlTable, 6);
    tablePtr->classCount = classCount;
    
#ifdef LOOKUP_TEST
    printf("\n       Total Classes: %d", classCount);
#endif
    
    SFUShort markArrayOffset = SFReadUShort(mtlTable, 8);
    
#ifdef LOOKUP_TEST
    printf("\n       Mark Array Table:");
    printf("\n        Offset: %d", markArrayOffset);
#endif
    
    SFReadMarkArray(&mtlTable[markArrayOffset], &tablePtr->markArray);
    
    SFUShort ligatureArrayOffset = SFReadUShort(mtlTable, 10);
    
#ifdef LOOKUP_TEST
    printf("\n       Ligature Array Table:");
    printf("\n        Offset: %d", ligatureArrayOffset);
#endif
    
    const SFUByte * const laTable = &mtlTable[ligatureArrayOffset];
    
    LigatureArrayTable ligatureArrayTable;
    
    SFUShort ligatureCount = SFReadUShort(laTable, 0);
    ligatureArrayTable.ligatureCount = ligatureCount;
    
#ifdef LOOKUP_TEST
    printf("\n        Total Ligature Attach Tables: %d", ligatureCount);
#endif
    
    LigatureAttachTable *ligatureAttachTables = malloc(sizeof(LigatureAttachTable) * ligatureCount);
    
    for (SFUShort i = 0; i < ligatureCount; i++) {
        SFUShort ligatureAttachOffset = SFReadUShort(laTable, 2 + (i * 2));
        
#ifdef LOOKUP_TEST
        printf("\n        Ligature Attach Table At Index %d:", i);
        printf("\n         Offset: %d", ligatureAttachOffset);
#endif
        
        const SFUByte * const latTable = &laTable[ligatureAttachOffset];
        
        SFUShort componentCount = SFReadUShort(latTable, 0);
        ligatureAttachTables[i].componentCount = componentCount;
        
#ifdef LOOKUP_TEST
        printf("\n         Total Component Records: %d", componentCount);
#endif
        
        ComponentRecord *componentRecords = malloc(sizeof(ComponentRecord) * componentCount);

        int beginOffset = 2;
        for (SFUShort j = 0; j < componentCount; j++) {
#ifdef LOOKUP_TEST
            printf("\n         Component Record At Index %d:", j);
            printf("\n          Total Ligature Anchor Tables: %d", classCount);
#endif
            
            //const UInt8 * const crTable = &latTable[2 + (j * componentCount)];
            
            AnchorTable *ligatureAnchors = malloc(sizeof(AnchorTable) * classCount);
            
            for (SFUShort k = 0; k < classCount; k++) {
                SFUShort ligatureAnchorOffset = SFReadUShort(latTable, beginOffset);
                beginOffset += 2;
            
#ifdef LOOKUP_TEST
                printf("\n          Ligature Anchor At Index %d:", k);
                printf("\n           Offset: %d", ligatureAnchorOffset);
#endif
            
                SFReadAnchorTable(&latTable[ligatureAnchorOffset], &ligatureAnchors[k]);
            }
            
            componentRecords[j].ligatureAnchor = ligatureAnchors;
        }
        
        ligatureAttachTables[i].componentRecord = componentRecords;
    }
    
    ligatureArrayTable.ligatureAttach = ligatureAttachTables;
    tablePtr->ligatureArray = ligatureArrayTable;
}

static void SFFreeMarkToLigatureAttachment(MarkToLigatureAttachmentPosSubtable *tablePtr) {
    SFFreeCoverageTable(&tablePtr->markCoverage);
    SFFreeCoverageTable(&tablePtr->ligatureCoverage);
    SFFreeMarkArray(&tablePtr->markArray);
    
    for (SFUShort i = 0; i < tablePtr->ligatureArray.ligatureCount; i++) {
        for (SFUShort j = 0; j < tablePtr->ligatureArray.ligatureAttach[i].componentCount; j++)
            free(tablePtr->ligatureArray.ligatureAttach[i].componentRecord[j].ligatureAnchor);
        
        free(tablePtr->ligatureArray.ligatureAttach[i].componentRecord);
    }
    
    free(tablePtr->ligatureArray.ligatureAttach);
}

#endif


#ifdef GPOS_MARK_TO_MARK

static void SFReadMarkToMarkAttachment(const SFUByte * const mtmTable, MarkToMarkAttachmentPosSubtable *tablePtr) {
    SFUShort posFormat = SFReadUShort(mtmTable, 0);
    tablePtr->posFormat = posFormat;
    
    SFUShort mark1CoverageOffset = SFReadUShort(mtmTable, 2);
    
#ifdef LOOKUP_TEST
    printf("\n      Mark To Mark Attachment Positioning Table:");
    printf("\n       Positioning Format: %d", posFormat);
    printf("\n       Mark 1 Coverage Table:");
    printf("\n        Offset: %d", mark1CoverageOffset);
#endif
    
    SFReadCoverageTable(&mtmTable[mark1CoverageOffset], &tablePtr->mark1Coverage);
    
    SFUShort mark2CoverageOffset = SFReadUShort(mtmTable, 4);
    
#ifdef LOOKUP_TEST
    printf("\n       Mark 2 Coverage Table:");
    printf("\n        Offset: %d", mark2CoverageOffset);
#endif
    
    SFReadCoverageTable(&mtmTable[mark2CoverageOffset], &tablePtr->mark2Coverage);
    
    SFUShort classCount = SFReadUShort(mtmTable, 6);
    tablePtr->classCount = classCount;
    
#ifdef LOOKUP_TEST
    printf("\n       Total Classes: %d", classCount);
#endif
    
    SFUShort mark1ArrayOffset = SFReadUShort(mtmTable, 8);
    
#ifdef LOOKUP_TEST
    printf("\n       Mark 1 Array Table:");
    printf("\n        Offset: %d", mark1ArrayOffset);
#endif
    
    SFReadMarkArray(&mtmTable[mark1ArrayOffset], &tablePtr->mark1Array);
    
    SFUShort mark2ArrayOffset = SFReadUShort(mtmTable, 10);
    
#ifdef LOOKUP_TEST
    printf("\n       Mark 2 Array Table:");
    printf("\n        Offset: %d", mark2ArrayOffset);
#endif
    
    const SFUByte * const m2aTable = &mtmTable[mark2ArrayOffset];
    
    Mark2ArrayTable mark2ArrayTable;
    
    SFUShort mark2Count = SFReadUShort(m2aTable, 0);
    mark2ArrayTable.mark2Count = mark2Count;
    
#ifdef LOOKUP_TEST
    printf("\n        Total Mark 2 Records: %d", mark2Count);
#endif
    
    Mark2Record *mark2Records = malloc(sizeof(Mark2Record) * mark2Count);
    
    SFUShort beginOffset = 2;
    for (SFUShort i = 0; i < mark2Count; i++) {
#ifdef LOOKUP_TEST
        printf("\n        Mark 2 Record At Index %d:", i);
        printf("\n         Total Mark 2 Anchor Tables: %d", classCount);
#endif

        AnchorTable *mark2Anchors = malloc(sizeof(AnchorTable) * classCount);
        
        SFUShort j = 0;
        for (; j < classCount; j++) {
            SFUShort mark2AnchorOffset = SFReadUShort(m2aTable, beginOffset);
            
#ifdef LOOKUP_TEST
            printf("\n         Mark 2 Anchor At Index: %d", j);
            printf("\n          Offset: %d", mark2AnchorOffset);
#endif
            
            SFReadAnchorTable(&m2aTable[mark2AnchorOffset], &mark2Anchors[j]);
            
            beginOffset += 2;
        }

        mark2Records[i].mark2Anchor = mark2Anchors;
    }
    
    mark2ArrayTable.mark2Record = mark2Records;
    tablePtr->mark2Array = mark2ArrayTable;
}

static void SFFreeMarkToMarkAttachment(MarkToMarkAttachmentPosSubtable *tablePtr) {
    SFFreeCoverageTable(&tablePtr->mark1Coverage);
    SFFreeCoverageTable(&tablePtr->mark2Coverage);
    SFFreeMarkArray(&tablePtr->mark1Array);
    
    for (SFUShort i = 0; i < tablePtr->mark2Array.mark2Count; i++)
        free(tablePtr->mark2Array.mark2Record[i].mark2Anchor);
    
    free(tablePtr->mark2Array.mark2Record);
}

#endif


#ifdef GPOS_CONTEXT

static void SFReadContextPos(const SFUByte * const cpTable, ContextPosSubtable *tablePtr) {
    SFUShort posFormat = SFReadUShort(cpTable, 0);
    tablePtr->posFormat = posFormat;
    
#ifdef LOOKUP_TEST
    printf("\n      Context Positioning Table:");
    printf("\n       Positioning Format: %d", posFormat);
#endif
    
    switch (posFormat) {
            
#ifdef GPOS_CONTEXT_FORMAT1
        case 1:
        {
            SFUShort coverageOffset = SFReadUShort(cpTable, 2);
            
#ifdef LOOKUP_TEST
            printf("\n       Coverage Table:");
            printf("\n        Offset: %d", coverageOffset);
#endif
            
            SFReadCoverageTable(&cpTable[coverageOffset], &tablePtr->format.format1.coverage);
            
            SFUShort posRuleSetCount = SFReadUShort(cpTable, 4);
            tablePtr->format.format1.posRuleSetCount = posRuleSetCount;
            
#ifdef LOOKUP_TEST
            printf("\n       Total Position Rule Sets: %d", posRuleSetCount);
#endif
            
            PosRuleSetTable *posRuleSetTables = malloc(sizeof(PosRuleSetTable) * posRuleSetCount);
            
            for (SFUShort i = 0; i < posRuleSetCount; i++) {
                SFUShort posRuleSetOffset = SFReadUShort(cpTable, 6 + (i * 2));
                const SFUByte * const srsTable = &cpTable[posRuleSetOffset];
                
#ifdef LOOKUP_TEST
                printf("\n       Position Rule Set %d:", i + 1);
                printf("\n        Offset: %d", posRuleSetOffset);
#endif
                
                SFUShort posRuleCount = SFReadUShort(srsTable, 0);
                posRuleSetTables[i].posRuleCount = posRuleCount;
                
#ifdef LOOKUP_TEST
                printf("\n        Total Position Rules: %d", posRuleCount);
#endif
                
                PosRuleTable *posRuleTables = malloc(sizeof(PosRuleTable) * posRuleCount);
                
                for (SFUShort j = 0; j < posRuleCount; j++) {
                    SFUShort posRuleOffset = SFReadUShort(srsTable, 2 + (j * 2));
                    const SFUByte * const srTable = &srsTable[posRuleOffset];
                    
#ifdef LOOKUP_TEST
                    printf("\n        Position Rule %d:", j + 1);
                    printf("\n         Offset: %d", posRuleOffset);
#endif
                    
                    SFUShort glyphCount = SFReadUShort(srTable, 0);
                    posRuleTables[j].glyphCount = glyphCount;
                    
                    SFUShort posCount = SFReadUShort(srTable, 2);
                    posRuleTables[j].posCount = posCount;
                    
#ifdef LOOKUP_TEST
                    printf("\n         Total Input Glyphs: %d", glyphCount);
                    printf("\n         Total Lookup Records: %d", posCount);
#endif
                    
                    SFGlyph *glyphs = malloc(sizeof(SFGlyph) * glyphCount);
                    glyphs[0] = 0;
                    
                    SFUShort k = 0;
                    for (; k < glyphCount; k++) {
                        glyphs[k + 1] = SFReadUShort(srTable, 4 + (k * 2));
                        
#ifdef LOOKUP_TEST
                        printf("\n         Input Glyph At Index %d: %d", k, glyphs[k + 1]);
#endif
                    }
                    
                    posRuleTables[j].input = glyphs;
                    
                    k += 4;
                    
                    PosLookupRecord *posLookupRecords = malloc(sizeof(PosLookupRecord) * posCount);
                    for (SFUShort l = 0; l < posCount; l++) {
                        SFUShort beginOffset = k + (l * 4);
                        
                        posLookupRecords[l].sequenceIndex = SFReadUShort(srTable, beginOffset);
                        posLookupRecords[l].lookupListIndex = SFReadUShort(srTable, beginOffset + 2);
                        
#ifdef LOOKUP_TEST
                        printf("\n         Lookup Record At Index %d:", l);
                        printf("\n          Sequence Index: %d", posLookupRecords[l].sequenceIndex);
                        printf("\n          Lookup List Index: %d", posLookupRecords[l].lookupListIndex);
#endif
                    }
                    
                    posRuleTables[j].posLookupRecord = posLookupRecords;
                }
                
                posRuleSetTables[i].posRule = posRuleTables;
            }
            
            tablePtr->format.format1.posRuleSet = posRuleSetTables;
        }
            break;
#endif
            
#ifdef GPOS_CONTEXT_FORMAT2
        case 2:
        {
            SFUShort coverageOffset = SFReadUShort(cpTable, 2);
            
#ifdef LOOKUP_TEST
            printf("\n       Coverage Table:");
            printf("\n        Offset: %d", coverageOffset);
#endif
            
            SFReadCoverageTable(&cpTable[coverageOffset], &tablePtr->format.format2.coverage);
            
            SFUShort classDefOffset = SFReadUShort(cpTable, 4);
            
#ifdef LOOKUP_TEST
            printf("\n       Class Definition Table:");
            printf("\n        Offset: %d", classDefOffset);
#endif
            
            SFReadClassDefTable(&cpTable[classDefOffset], &tablePtr->format.format2.classDef);
            
            SFUShort posClassSetCount = SFReadUShort(cpTable, 6);
            
#ifdef LOOKUP_TEST
            printf("\n       Total Pos Class Sets: %d", posClassSetCount);
#endif
            
            PosClassSetTable *posRuleSetTables = malloc(sizeof(PosClassSetTable) * posClassSetCount);
            
            for (SFUShort i = 0; i < posClassSetCount; i++) {
                SFUShort posClassSetOffset = SFReadUShort(cpTable, 8 + (i * 2));
                const SFUByte * const scpTable = &cpTable[posClassSetOffset];
                
#ifdef LOOKUP_TEST
                printf("\n       Pos Class Set %d:", i + 1);
                printf("\n        Offset: %d", posClassSetOffset);
#endif
                
                SFUShort posClassRuleCount = SFReadUShort(scpTable, 0);
                posRuleSetTables[i].posClassRuleCnt = posClassRuleCount;
                
#ifdef LOOKUP_TEST
                printf("\n        Total Pos Class Rules: %d", posClassRuleCount);
#endif
                
                PosClassRuleTable *posRuleTables = malloc(sizeof(PosClassRuleTable) * posClassRuleCount);
                
                for (SFUShort j = 0; j < posClassRuleCount; j++) {
                    SFUShort posClassRuleOffset = SFReadUShort(scpTable, 2 + (j * 2));
                    const SFUByte * const scrTable = &scpTable[posClassRuleOffset];
                    
#ifdef LOOKUP_TEST
                    printf("\n        Pos Class Rule %d:", j + 1);
                    printf("\n         Offset: %d", posClassRuleOffset);
#endif
                    
                    SFUShort glyphCount = SFReadUShort(scrTable, 0);
                    posRuleTables[j].glyphCount = glyphCount;
                    
                    SFUShort posCount = SFReadUShort(scrTable, 2);
                    posRuleTables[j].posCount = posCount;
                    
#ifdef LOOKUP_TEST
                    printf("\n         Total Input Glyphs: %d", glyphCount);
                    printf("\n         Total Lookup Records: %d", posCount);
#endif
                    
                    SFUShort *classes = malloc(sizeof(SFUShort) * glyphCount);
                    classes[0] = 0;
                    
                    SFUShort k = 0;
                    for (; k < glyphCount; k++) {
                        classes[k + 1] = SFReadUShort(scrTable, 4 + (k * 2));
                        
#ifdef LOOKUP_TEST
                        printf("\n         Class At Index %d: %d", k + 1, classes[k + 1]);
#endif
                    }
                    
                    posRuleTables[j].cls = classes;
                    
                    k += 4;
                    
                    PosLookupRecord *posLookupRecords = malloc(sizeof(PosLookupRecord) * posCount);
                    for (SFUShort l = 0; l < posCount; l++) {
                        SFUShort beginOffset = k + (l * 4);
                        
                        posLookupRecords[l].sequenceIndex = SFReadUShort(scrTable, beginOffset);
                        posLookupRecords[l].lookupListIndex = SFReadUShort(scrTable, beginOffset + 2);
                        
#ifdef LOOKUP_TEST
                        printf("\n         Lookup Record At Index %d:", l);
                        printf("\n          Sequence Index: %d", posLookupRecords[l].sequenceIndex);
                        printf("\n          Lookup List Index: %d", posLookupRecords[l].lookupListIndex);
#endif
                    }
                    
                    posRuleTables[j].posLookupRecord = posLookupRecords;
                }
                
                posRuleSetTables[i].posClassRule = posRuleTables;
            }
            
            tablePtr->format.format2.posClassSet = posRuleSetTables;
        }
            break;
#endif
            
#ifdef GPOS_CONTEXT_FORMAT3
        case 3:
        {
            SFUShort glyphCount = SFReadUShort(cpTable, 2);
            SFUShort posCount = SFReadUShort(cpTable, 4);
            
#ifdef LOOKUP_TEST
            printf("\n       Total Coverage Tables: %d", glyphCount);
            printf("\n       Total Lookup Records: %d", posCount);
#endif
            
            tablePtr->format.format3.glyphCount = glyphCount;
            tablePtr->format.format3.posCount = posCount;
            
            CoverageTable *coverageTables = malloc(sizeof(CoverageTable) * glyphCount);
            
            for (SFUShort i = 0; i < glyphCount; i++) {
                SFUShort coverageOffset = SFReadUShort(cpTable, 6 + (i * 2));
                
#ifdef LOOKUP_TEST
                printf("\n       Coverage Table %d:", i + 1);
                printf("\n        Offset: %d", coverageOffset);
#endif
                
                SFReadCoverageTable(&cpTable[coverageOffset], &coverageTables[i]);
            }
            
            tablePtr->format.format3.coverage = coverageTables;
            
            PosLookupRecord *posLookupRecords = malloc(sizeof(PosLookupRecord) * posCount);
            for (SFUShort j = 0; j < posCount; j++) {
                SFUShort beginOffset = 8 + (j * 4);
                
                posLookupRecords[j].sequenceIndex = SFReadUShort(cpTable, beginOffset);
                posLookupRecords[j].lookupListIndex = SFReadUShort(cpTable, beginOffset + 2);
                
#ifdef LOOKUP_TEST
                printf("\n       Lookup Record At Index %d:", j);
                printf("\n        Sequence Index: %d", posLookupRecords[j].sequenceIndex);
                printf("\n        Lookup List Index: %d", posLookupRecords[j].lookupListIndex);
#endif
            }
            
            tablePtr->format.format3.posLookupRecord = posLookupRecords;
        }
            break;
#endif
    }
}

static void SFFreeContextPos(ContextPosSubtable *tablePtr) {
    switch (tablePtr->posFormat) {
            
#ifdef GPOS_CONTEXT_FORMAT1
        case 1:
            SFFreeCoverageTable(&tablePtr->format.format1.coverage);
            
            for (int i = 0; i < tablePtr->format.format1.posRuleSetCount; i++) {
                for (int j = 0; j < tablePtr->format.format1.posRuleSet[i].posRuleCount; j++) {
                    free(tablePtr->format.format1.posRuleSet[i].posRule[j].input);
                    free(tablePtr->format.format1.posRuleSet[i].posRule[j].posLookupRecord);
                }
                
                free(tablePtr->format.format1.posRuleSet[i].posRule);
            }
            
            free(tablePtr->format.format1.posRuleSet);
            
            break;
#endif
            
#ifdef GPOS_CONTEXT_FORMAT2
        case 2:
            SFFreeCoverageTable(&tablePtr->format.format2.coverage);
            
            for (int i = 0; i < tablePtr->format.format2.posClassSetCnt; i++) {
                for (int j = 0; j < tablePtr->format.format2.posClassSet[i].posClassRuleCnt; j++) {
                    free(tablePtr->format.format2.posClassSet[i].posClassRule[j].cls);
                    free(tablePtr->format.format2.posClassSet[i].posClassRule[j].posLookupRecord);
                }
                
                free(tablePtr->format.format2.posClassSet[i].posClassRule);
            }
            
            SFFreeClassDefTable(&tablePtr->format.format2.classDef);
            free(tablePtr->format.format2.posClassSet);
            
            break;
#endif
            
#ifdef GPOS_CONTEXT_FORMAT3
        case 3:
            for (int i = 0; i < tablePtr->format.format3.glyphCount; i++)
                SFFreeCoverageTable(&tablePtr->format.format3.coverage[i]);
            
            free(tablePtr->format.format3.coverage);
            free(tablePtr->format.format3.posLookupRecord);
            
            break;
#endif
    }
}

#endif


#ifdef GPOS_CHAINING_CONTEXT

static void SFReadChainingContextPos(const SFUByte * const ccpTable, ChainingContextualPosSubtable *tablePtr) {
    SFUShort posFormat = SFReadUShort(ccpTable, 0);
    tablePtr->posFormat = posFormat;
    
#ifdef LOOKUP_TEST
    printf("\n      Chaining Context Positioning Table:");
    printf("\n       Positioning Format: %d", posFormat);
#endif
    
    switch (posFormat) {
            
#ifdef GPOS_CHAINING_CONTEXT_FORMAT1
        case 1:
        {
            SFUShort coverageOffset = SFReadUShort(ccpTable, 2);
            
#ifdef LOOKUP_TEST
            printf("\n       Coverage Table:");
            printf("\n        Offset: %d", coverageOffset);
#endif
            SFReadCoverageTable(&ccpTable[coverageOffset], &tablePtr->format.format1.coverage);
            
            SFUShort chainPosRuleSetCount = SFReadUShort(ccpTable, 4);
            
#ifdef LOOKUP_TEST
            printf("\n       Total Chain Pos Rule Sets: %d", chainPosRuleSetCount);
#endif
            tablePtr->format.format1.chainPosRuleSetCount = chainPosRuleSetCount;
            
            ChainPosRuleSetTable *chainPosRuleSetTables = malloc(sizeof(ChainPosRuleSetTable) * chainPosRuleSetCount);
            
            for (SFUShort i = 0; i < chainPosRuleSetCount; i++) {
                SFUShort chainPosRuleSetOffset = SFReadUShort(ccpTable, 6 + (i * 2));
                const SFUByte * const csrsTable = &ccpTable[chainPosRuleSetOffset];
                
#ifdef LOOKUP_TEST
                printf("\n       Chain Pos Rule Set %d:", i + 1);
                printf("\n        Offset: %d", chainPosRuleSetOffset);
#endif
                
                SFUShort chainPosRuleCount = SFReadUShort(csrsTable, 0);
                chainPosRuleSetTables[i].chainPosRuleCount = chainPosRuleCount;
                
#ifdef LOOKUP_TEST
                printf("\n        Total Chain Pos Rule Tables: %d", chainPosRuleCount);
#endif
                
                ChainPosRuleSubtable *chainPosRuleTables = malloc(sizeof(ChainPosRuleSubtable) * chainPosRuleCount);
                
                for (SFUShort j = 0; j < chainPosRuleCount; j++) {
                    SFUShort chainPosRuleOffset = SFReadUShort(csrsTable, 2 + (j * 2));
                    const SFUByte * const csrTable = &csrsTable[chainPosRuleOffset];
                    
#ifdef LOOKUP_TEST
                    printf("\n        Chain Pos Rule Table %d:", j + 1);
                    printf("\n         Offset: %d", chainPosRuleOffset);
#endif
                    
                    SFUShort backtrackGlyphCount = SFReadUShort(csrTable, 0);
                    chainPosRuleTables[j].backtrackGlyphCount = backtrackGlyphCount;
                    
#ifdef LOOKUP_TEST
                    printf("\n         Total Backtrack Glyphs: %d", backtrackGlyphCount);
#endif
                    
                    SFGlyph *backtrackGlyphs = malloc(sizeof(SFGlyph) * backtrackGlyphCount);
                    
                    SFUShort k = 0;
                    for (; k < backtrackGlyphCount; k++) {
                        backtrackGlyphs[k] = SFReadUShort(csrTable, 2 + (k * 2));
                        
#ifdef LOOKUP_TEST
                        printf("\n         Backtrack Glyph At Index %d: %d", k, backtrackGlyphs[k]);
#endif
                    }
                    k *= 2;
                    
                    chainPosRuleTables[j].backtrack = backtrackGlyphs;
                    
                    SFUShort inputGlyphCount = SFReadUShort(csrTable, k);
                    chainPosRuleTables[j].inputGlyphCount = inputGlyphCount;
                    
#ifdef LOOKUP_TEST
                    printf("\n         Total Input Glyphs: %d", inputGlyphCount);
#endif
                    
                    SFGlyph *inputGlyphs = malloc(sizeof(SFGlyph) * inputGlyphCount);
                    inputGlyphs[0] = 0;
                    
                    k += 2;
                    SFUShort l = 0;
                    for (; l < inputGlyphCount - 1; l++) {
                        inputGlyphs[l + 1] = SFReadUShort(csrTable, k + (l * 2));
                        
#ifdef LOOKUP_TEST
                        printf("\n         Input Glyph At Index %d: %d", l + 1, inputGlyphs[l + 1]);
#endif
                    }
                    k += l * 2;
                    
                    chainPosRuleTables[j].input = inputGlyphs;
                    
                    SFUShort lookaheadGlyphCount = SFReadUShort(csrTable, k);
                    chainPosRuleTables[j].lookaheadGlyphCount = lookaheadGlyphCount;
                    
#ifdef LOOKUP_TEST
                    printf("\n         Total Lookahead Glyphs: %d", lookaheadGlyphCount);
#endif
                    
                    SFGlyph *lookaheadGlyphs = malloc(sizeof(SFGlyph) * lookaheadGlyphCount);
                    
                    k += 2;
                    SFUShort m = 0;
                    for (; m < lookaheadGlyphCount; m++) {
                        lookaheadGlyphs[m] = SFReadUShort(csrTable, k + (m * 2));
                        
#ifdef LOOKUP_TEST
                        printf("\n         Lookahead Glyph At Index %d: %d", m, inputGlyphs[m]);
#endif
                    }
                    k += m * 2;
                    
                    chainPosRuleTables[j].lookAhead = lookaheadGlyphs;
                    
                    SFUShort posCount = SFReadUShort(csrTable, k);
                    
#ifdef LOOKUP_TEST
                    printf("\n         Total Positioning Lookup Records: %d", posCount);
#endif
                    
                    PosLookupRecord *posLookupRecords = malloc(sizeof(PosLookupRecord) * posCount);
                    
                    for (SFUShort n = 0; n < posCount; l++) {
                        SFUShort beginOffset = k + (n * 4);
                        
                        posLookupRecords[n].sequenceIndex = SFReadUShort(csrTable, beginOffset);
                        posLookupRecords[n].lookupListIndex = SFReadUShort(csrTable, beginOffset + 2);
                        
#ifdef LOOKUP_TEST
                        printf("\n         Lookup Record At Index %d:", n);
                        printf("\n          Sequence Index: %d", posLookupRecords[n].sequenceIndex);
                        printf("\n          Lookup List Index: %d", posLookupRecords[n].lookupListIndex);
#endif
                    }
                    
                    chainPosRuleTables[j].posLookupRecord = posLookupRecords;
                }
                
                chainPosRuleSetTables[i].chainPosRule = chainPosRuleTables;
            }
            
            tablePtr->format.format1.chainPosRuleSet = chainPosRuleSetTables;
        }
            break;
#endif
            
#ifdef GPOS_CHAINING_CONTEXT_FORMAT2
        case 2:
        {
            SFUShort coverageOffset = SFReadUShort(ccpTable, 2);
            
#ifdef LOOKUP_TEST
            printf("\n       Coverage Table:");
            printf("\n        Offset: %d", coverageOffset);
#endif
            
            SFReadCoverageTable(&ccpTable[coverageOffset], &tablePtr->format.format2.coverage);
            
            SFUShort backtrackClassDefOffset = SFReadUShort(ccpTable, 4);
            
#ifdef LOOKUP_TEST
            printf("\n       Backtrack Class Definition:");
            printf("\n        Offset: %d", backtrackClassDefOffset);
#endif
            
            SFReadClassDefTable(&ccpTable[backtrackClassDefOffset], &tablePtr->format.format2.backtrackClassDef);
            
            SFUShort inputClassDefOffset = SFReadUShort(ccpTable, 6);
            
#ifdef LOOKUP_TEST
            printf("\n       Input Class Definition:");
            printf("\n        Offset: %d", inputClassDefOffset);
#endif
            
            SFReadClassDefTable(&ccpTable[inputClassDefOffset], &tablePtr->format.format2.inputClassDef);
            
            SFUShort lookaheadClassDefOffset = SFReadUShort(ccpTable, 8);
            
#ifdef LOOKUP_TEST
            printf("\n       Lookahead Class Definition:");
            printf("\n        Offset: %d", lookaheadClassDefOffset);
#endif
            
            SFReadClassDefTable(&ccpTable[lookaheadClassDefOffset], &tablePtr->format.format2.lookaheadClassDef);
            
            SFUShort chainPosClassSetCount = SFReadUShort(ccpTable, 10);
            
#ifdef LOOKUP_TEST
            printf("\n       Total Chain Pos Class Sets: %d", chainPosClassSetCount);
#endif
            
            ChainPosClassSetTable *chainPosClassSets = malloc(sizeof(ChainPosClassSetTable) * chainPosClassSetCount);
            
            for (SFUShort i = 0; i < chainPosClassSetCount; i++) {
                SFUShort chainPosClassSetOffset = SFReadUShort(ccpTable, 12 + (i * 2));
                const SFUByte * const cscpTable = &ccpTable[chainPosClassSetOffset];
                
#ifdef LOOKUP_TEST
                printf("\n       Chain Pos Class Set %d:", i + 1);
                printf("\n        Offset: %d", chainPosClassSetOffset);
#endif
                
                SFUShort chainPosClassRuleCount = SFReadUShort(cscpTable, 0);
                chainPosClassSets[i].ChainPosClassRuleCnt = chainPosClassRuleCount;
                
#ifdef LOOKUP_TEST
                printf("\n        Total Chain Pos Class Rules: %d", chainPosClassRuleCount);
#endif
                
                ChainPosClassRuleSubTable *chainPosClassRuleTables = malloc(sizeof(ChainPosClassRuleSubTable) * chainPosClassRuleCount);
                
                for (SFUShort j = 0; j < chainPosClassRuleCount; j++) {
                    SFUShort chainPosClassRuleOffset = SFReadUShort(cscpTable, 2 + (j * 2));
                    const SFUByte * const cscrTable = &cscpTable[chainPosClassRuleOffset];
                    
#ifdef LOOKUP_TEST
                    printf("\n        Chain Pos Class Rule %d:", j + 1);
                    printf("\n         Offset: %d", chainPosClassRuleOffset);
#endif
                    
                    SFUShort backtrackGlyphCount = SFReadUShort(cscrTable, 0);
                    chainPosClassRuleTables[j].backtrackGlyphCount = backtrackGlyphCount;
                    
#ifdef LOOKUP_TEST
                    printf("\n         Total Backtrack Glyphs: %d", backtrackGlyphCount);
#endif
                    
                    SFGlyph *backtrackGlyphs = malloc(sizeof(SFGlyph) * backtrackGlyphCount);
                    
                    SFUShort k = 0;
                    for (; k < backtrackGlyphCount; k++) {
                        backtrackGlyphs[k] = SFReadUShort(cscrTable, 2 + (k * 2));
                        
#ifdef LOOKUP_TEST
                        printf("\n         Backtrack Glyph At Index %d: %d", k, backtrackGlyphs[k]);
#endif
                    }
                    k *= 2;
                    
                    chainPosClassRuleTables[j].backtrack = backtrackGlyphs;
                    
                    SFUShort inputGlyphCount = SFReadUShort(cscrTable, k);
                    chainPosClassRuleTables[j].inputGlyphCount = inputGlyphCount;
                    
#ifdef LOOKUP_TEST
                    printf("\n         Total Input Glyphs: %d", inputGlyphCount);
#endif
                    
                    SFGlyph *inputGlyphs = malloc(sizeof(SFGlyph) * inputGlyphCount);
                    inputGlyphs[0] = 0;
                    
                    k += 2;
                    SFUShort l = 0;
                    for (; l < inputGlyphCount - 1; l++) {
                        inputGlyphs[l + 1] = SFReadUShort(cscrTable, k + (l * 2));
                        
#ifdef LOOKUP_TEST
                        printf("\n         Input Glyph At Index %d: %d", l + 1, inputGlyphs[l + 1]);
#endif
                    }
                    k += l * 2;
                    
                    chainPosClassRuleTables[j].input = inputGlyphs;
                    
                    SFUShort lookaheadGlyphCount = SFReadUShort(cscrTable, k);
                    chainPosClassRuleTables[j].lookaheadGlyphCount = lookaheadGlyphCount;
                    
#ifdef LOOKUP_TEST
                    printf("\n         Total Lookahead Glyphs: %d", lookaheadGlyphCount);
#endif
                    
                    SFGlyph *lookaheadGlyphs = malloc(sizeof(SFGlyph) * lookaheadGlyphCount);
                    
                    k += 2;
                    SFUShort m = 0;
                    for (; m < lookaheadGlyphCount; m++) {
                        lookaheadGlyphs[m] = SFReadUShort(cscrTable, k + (m * 2));
                        
#ifdef LOOKUP_TEST
                        printf("\n         Lookahead Glyph At Index %d: %d", m, lookaheadGlyphs[m]);
#endif
                    }
                    k += m * 2;
                    
                    chainPosClassRuleTables[j].lookAhead = lookaheadGlyphs;
                    
                    SFUShort posCount = SFReadUShort(cscrTable, k);
                    
#ifdef LOOKUP_TEST
                    printf("\n         Total Positioning Lookup Records: %d", posCount);
#endif
                    
                    PosLookupRecord posLookupRecords[posCount];
                    
                    for (SFUShort n = 0; n < posCount; l++) {
                        SFUShort beginOffset = k + (n * 4);
                        
                        posLookupRecords[n].sequenceIndex = SFReadUShort(cscrTable, beginOffset);
                        posLookupRecords[n].lookupListIndex = SFReadUShort(cscrTable, beginOffset + 2);
                        
#ifdef LOOKUP_TEST
                        printf("\n         Lookup Record At Index %d:", n);
                        printf("\n          Sequence Index: %d", posLookupRecords[n].sequenceIndex);
                        printf("\n          Lookup List Index: %d", posLookupRecords[n].lookupListIndex);
#endif
                    }
                    
                    chainPosClassRuleTables[j].posLookupRecord = posLookupRecords;
                }
                
                chainPosClassSets[i].chainPosClassRule = chainPosClassRuleTables;
            }
            
            tablePtr->format.format2.chainPosClassSet = chainPosClassSets;
        }
            break;
#endif
            
#ifdef GPOS_CHAINING_CONTEXT_FORMAT3
        case 3:
        {
            SFUShort backtrackGlyphCount = SFReadUShort(ccpTable, 2);
            tablePtr->format.format3.backtrackGlyphCount = backtrackGlyphCount;
            
#ifdef LOOKUP_TEST
            printf("\n       Total Backtrack Glyphs: %d", backtrackGlyphCount);
#endif
            
            CoverageTable *backtrackCoverageTables = malloc(sizeof(CoverageTable) * backtrackGlyphCount);
            
            SFUShort i = 0;
            for (; i < backtrackGlyphCount; i++) {
                SFUShort coverageOffset = SFReadUShort(ccpTable, 4 + (i * 2));
                
#ifdef LOOKUP_TEST
                printf("\n        Backtrack Coverage %d:", i + 1);
                printf("\n         Offset: %d", coverageOffset);
                
#endif
                SFReadCoverageTable(&ccpTable[coverageOffset], &backtrackCoverageTables[i]);
            }
            i = (4 + (i * 2));
            
            tablePtr->format.format3.backtrackGlyphCoverage = backtrackCoverageTables;
            
            SFUShort inputGlyphCount = SFReadUShort(ccpTable, i);
            tablePtr->format.format3.inputGlyphCount = inputGlyphCount;
            
#ifdef LOOKUP_TEST
            printf("\n       Total Input Glyphs: %d", inputGlyphCount);
#endif
            
            CoverageTable *inputCoverageTables = malloc(sizeof(CoverageTable) * inputGlyphCount);
            
            i += 2;
            
            SFUShort j = 0;
            for (; j < inputGlyphCount; j++) {
                SFUShort coverageOffset = SFReadUShort(ccpTable, i + (j * 2));
                
#ifdef LOOKUP_TEST
                printf("\n        Input Coverage %d:", j);
                printf("\n         Offset: %d", coverageOffset);
#endif
                
                SFReadCoverageTable(&ccpTable[coverageOffset], &inputCoverageTables[j]);
            }
            i += (j * 2);
            
            tablePtr->format.format3.inputGlyphCoverage = inputCoverageTables;
            
            SFUShort lookaheadGlyphCount = SFReadUShort(ccpTable, i);
            tablePtr->format.format3.lookaheadGlyphCount = lookaheadGlyphCount;
            
#ifdef LOOKUP_TEST
            printf("\n       Total Lookahead Glyphs: %d", lookaheadGlyphCount);
#endif
            
            CoverageTable *lookaheadCoverageTables = malloc(sizeof(CoverageTable) * lookaheadGlyphCount);
            
            i += 2;
            
            SFUShort k = 0;
            for (; k < lookaheadGlyphCount; k++) {
                SFUShort coverageOffset = SFReadUShort(ccpTable, i + (k * 2));
                
#ifdef LOOKUP_TEST
                printf("\n        Lookahead Coverage %d:", k);
                printf("\n         Offset: %d", coverageOffset);
                
#endif
                SFReadCoverageTable(&ccpTable[coverageOffset], &lookaheadCoverageTables[k]);
            }
            i += (k * 2);
            
            tablePtr->format.format3.lookaheadGlyphCoverage = lookaheadCoverageTables;
            
            SFUShort posCount = SFReadUShort(ccpTable, i);
            tablePtr->format.format3.posCount = posCount;
            
            PosLookupRecord *posLookupRecords = malloc(sizeof(PosLookupRecord) * posCount);
            
#ifdef LOOKUP_TEST
            printf("\n         Total Positioning Lookup Records: %d", posCount);
#endif
            
            i += 2;
            for (SFUShort l = 0; l < posCount; l++) {
                SFUShort beginOffset = i + (l * 4);
                
                posLookupRecords[l].sequenceIndex = SFReadUShort(ccpTable, beginOffset);
                posLookupRecords[l].lookupListIndex = SFReadUShort(ccpTable, beginOffset + 2);
                
#ifdef LOOKUP_TEST
                printf("\n         Lookup Record At Index %d:", l);
                printf("\n          Sequence Index: %d", posLookupRecords[l].sequenceIndex);
                printf("\n          Lookup List Index: %d", posLookupRecords[l].lookupListIndex);
#endif
            }
            
            tablePtr->format.format3.PosLookupRecord = posLookupRecords;
        }
            break;
#endif
    }
}

static void SFFreeChainingContextPos(ChainingContextualPosSubtable *tablePtr) {
    switch (tablePtr->posFormat) {
            
#ifdef GPOS_CHAINING_CONTEXT_FORMAT1
        case 1:
            SFFreeCoverageTable(&tablePtr->format.format1.coverage);
            
            for (int i = 0; i < tablePtr->format.format1.chainPosRuleSetCount; i++) {
                for (int j = 0; j < tablePtr->format.format1.chainPosRuleSet[i].chainPosRuleCount; j++) {
                    free(tablePtr->format.format1.chainPosRuleSet[i].chainPosRule[j].backtrack);
                    free(tablePtr->format.format1.chainPosRuleSet[i].chainPosRule[j].input);
                    free(tablePtr->format.format1.chainPosRuleSet[i].chainPosRule[j].lookAhead);
                    free(tablePtr->format.format1.chainPosRuleSet[i].chainPosRule[j].posLookupRecord);
                }
                
                free(tablePtr->format.format1.chainPosRuleSet[i].chainPosRule);
            }
            
            free(tablePtr->format.format1.chainPosRuleSet);
            
            break;
#endif
            
#ifdef GPOS_CHAINING_CONTEXT_FORMAT2
        case 2:
            SFFreeCoverageTable(&tablePtr->format.format2.coverage);
            
            SFFreeClassDefTable(&tablePtr->format.format2.backtrackClassDef);
            SFFreeClassDefTable(&tablePtr->format.format2.inputClassDef);
            SFFreeClassDefTable(&tablePtr->format.format2.lookaheadClassDef);
            
            for (int i = 0; i < tablePtr->format.format2.chainPosClassSetCnt; i++) {
                for (int j = 0; j < tablePtr->format.format2.chainPosClassSet[i].ChainPosClassRuleCnt; j++) {
                    free(tablePtr->format.format2.chainPosClassSet[i].chainPosClassRule[j].backtrack);
                    free(tablePtr->format.format2.chainPosClassSet[i].chainPosClassRule[j].input);
                    free(tablePtr->format.format2.chainPosClassSet[i].chainPosClassRule[j].lookAhead);
                    free(tablePtr->format.format2.chainPosClassSet[i].chainPosClassRule[j].posLookupRecord);
                }
                
                free(tablePtr->format.format2.chainPosClassSet[i].chainPosClassRule);
            }
            
            free(tablePtr->format.format2.chainPosClassSet);
            
            break;
#endif
            
#ifdef GPOS_CHAINING_CONTEXT_FORMAT3
        case 3:
            for (int i = 0; i < tablePtr->format.format3.backtrackGlyphCount; i++)
                SFFreeCoverageTable(&tablePtr->format.format3.backtrackGlyphCoverage[i]);
            
            free(tablePtr->format.format3.backtrackGlyphCoverage);
            
            for (int i = 0; i < tablePtr->format.format3.inputGlyphCount; i++)
                SFFreeCoverageTable(&tablePtr->format.format3.inputGlyphCoverage[i]);
            
            free(tablePtr->format.format3.inputGlyphCoverage);
            
            for (int i = 0; i < tablePtr->format.format3.lookaheadGlyphCount; i++)
                SFFreeCoverageTable(&tablePtr->format.format3.lookaheadGlyphCoverage[i]);
            
            free(tablePtr->format.format3.lookaheadGlyphCoverage);
            free(tablePtr->format.format3.PosLookupRecord);
            
            break;
#endif
    }
}

#endif


static void *SFReadPositioning(const SFUByte * const sTable, LookupType *type) {

    if (*type == ltpExtensionPositioning) {
        *type = SFReadUShort(sTable, 2);
        SFUInt extensionOffset = SFReadUInt(sTable, 4);
        
        return SFReadPositioning(&sTable[extensionOffset], type);
    }
    
    void *subtablePtr = NULL;
    
    switch (*type) {
            
#ifdef GPOS_SINGLE
        case ltpSingleAdjustment:
        {
            SingleAdjustmentPosSubtable *subtable = malloc(sizeof(SingleAdjustmentPosSubtable));
            SFReadSingleAdjustment(sTable, subtable);
            subtablePtr = subtable;
            break;
        }
#endif
            
#ifdef GPOS_PAIR
        case ltpPairAdjustment:
        {
            PairAdjustmentPosSubtable *subtable = malloc(sizeof(PairAdjustmentPosSubtable));
            SFReadPairAdjustment(sTable, subtable);
            subtablePtr = subtable;
            break;
        }
#endif
            
#ifdef GPOS_CURSIVE
        case ltpCursiveAttachment:
        {
            CursiveAttachmentPosSubtable *subtable = malloc(sizeof(CursiveAttachmentPosSubtable));
            SFReadCursiveAttachment(sTable, subtable);
            subtablePtr = subtable;
            break;
        }
#endif
            
#ifdef GPOS_MARK_TO_BASE
        case ltpMarkToBaseAttachment:
        {
            MarkToBaseAttachmentPosSubtable *subtable = malloc(sizeof(MarkToBaseAttachmentPosSubtable));
            SFReadMarkToBaseAttachment(sTable, subtable);
            subtablePtr = subtable;
            break;
        }
#endif
            
#ifdef GPOS_MARK_TO_LIGATURE
        case ltpMarkToLigatureAttachment:
        {
            MarkToLigatureAttachmentPosSubtable *subtable = malloc(sizeof(MarkToLigatureAttachmentPosSubtable));
            SFReadMarkToLigatureAttachment(sTable, subtable);
            subtablePtr = subtable;
            break;
        }
#endif
            
#ifdef GPOS_MARK_TO_MARK
        case ltpMarkToMarkAttachment:
        {
            MarkToMarkAttachmentPosSubtable *subtable = malloc(sizeof(MarkToMarkAttachmentPosSubtable));
            SFReadMarkToMarkAttachment(sTable, subtable);
            subtablePtr = subtable;
            break;
        }
#endif
            
#ifdef GPOS_CONTEXT
        case ltpContextPositioning:
        {
            ContextPosSubtable *subtable = malloc(sizeof(ContextPosSubtable));
            SFReadContextPos(sTable, subtable);
            subtablePtr = subtable;
            break;
        }
#endif
            
#ifdef GPOS_CHAINING_CONTEXT
        case ltpChainedContextPositioning:
        {
            ChainingContextualPosSubtable *subtable = malloc(sizeof(ChainingContextualPosSubtable));
            SFReadChainingContextPos(sTable, subtable);
            subtablePtr = subtable;
            break;
        }
#endif

        default:
            //Reserved for future use
            break;
    }
    
    return subtablePtr;
}

static void SFFreePositioning(void *tablePtr, LookupType type) {
    switch (type) {
            
#ifdef GPOS_SINGLE
        case ltpSingleAdjustment:
            SFFreeSingleAdjustment(tablePtr);
            break;
#endif
            
#ifdef GPOS_PAIR
        case ltpPairAdjustment:
            SFFreePairAdjustment(tablePtr);
            break;
#endif
            
#ifdef GPOS_CURSIVE
        case ltpCursiveAttachment:
            SFFreeCursiveAttachment(tablePtr);
            break;
#endif
            
#ifdef GPOS_MARK_TO_BASE
        case ltpMarkToBaseAttachment:
            SFFreeMarkToBaseAttachment(tablePtr);
            break;
#endif
            
#ifdef GPOS_MARK_TO_LIGATURE
        case ltpMarkToLigatureAttachment:
            SFFreeMarkToLigatureAttachment(tablePtr);
            break;
#endif
            
#ifdef GPOS_MARK_TO_MARK
        case ltpMarkToMarkAttachment:
            SFFreeMarkToMarkAttachment(tablePtr);
            break;
#endif

#ifdef GPOS_CONTEXT
        case ltpContextPositioning:
            SFFreeContextPos(tablePtr);
            break;
#endif

#ifdef GPOS_CHAINING_CONTEXT
        case ltpChainedContextPositioning:
            SFFreeChainingContextPos(tablePtr);
            break;
#endif

        default:
            break;
    }
    
    free(tablePtr);
}


void SFReadGPOS(const SFUByte * const table, SFTableGPOS *tablePtr) {
    gposTable = (void **)&table;

    tablePtr->version = SFReadUInt(table, 0);
    
    SFUShort scriptListOffset = SFReadUShort(table, 4);
    
#ifdef SCRIPT_TEST
    printf("\nGPOS Header:");
    printf("\n Header Version: %u", tablePtr->version);
    printf("\n Script List:");
    printf("\n  Offset: %d", scriptListOffset);
#endif
    
    SFReadScriptListTable(&table[scriptListOffset], &tablePtr->scriptList);
    
    SFUShort featureListOffset = SFReadUShort(table, 6);
    
#ifdef FEATURE_TEST
    printf("\n Feature List:");
    printf("\n  Offset: %d", featureListOffset);
#endif
    
    SFReadFeatureListTable(&table[featureListOffset], &tablePtr->featureList);
    
    SFUShort lookupListOffset = SFReadUShort(table, 8);
    
#ifdef LOOKUP_TEST
    printf("\n Lookup List:");
    printf("\n  Offset: %d", lookupListOffset);
#endif
    
    SFReadLookupListTable(&table[lookupListOffset], &tablePtr->lookupList, &SFReadPositioning);
}

void SFFreeGPOS(SFTableGPOS *tablePtr) {
    SFFreeScriptListTable(&tablePtr->scriptList);
    SFFreeFeatureListTable(&tablePtr->featureList);
    SFFreeLookupListTable(&tablePtr->lookupList, &SFFreePositioning);
}
