#!/usr/bin/env python3
"""
Script to help migrate defineEditorAttribute() to new attribute system.

This script analyzes C++ files to find defineEditorAttribute implementations
and generates migration code for common patterns.
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass


@dataclass
class AttributePattern:
    """Represents a detected attribute pattern that can be migrated."""
    field_name: str
    attribute_type: str  # e.g., "PdmUiDoubleSliderEditorAttribute", "PdmUiPushButtonEditorAttribute"
    attributes: Dict[str, str]  # attribute name -> value expression
    is_static: bool  # True if values are constant, False if they depend on runtime state
    condition: Optional[str] = None  # Any if condition wrapping this


@dataclass
class FileAnalysis:
    """Analysis results for a single file."""
    filepath: str
    has_define_editor_attribute: bool
    patterns: List[AttributePattern]
    needs_manual_review: bool
    manual_review_reason: str = ""


def find_files_with_define_editor_attribute(base_path: str) -> List[str]:
    """Find all C++ files that contain defineEditorAttribute."""
    files = []

    for root, dirs, filenames in os.walk(base_path):
        # Skip certain directories
        if 'build' in root or 'ThirdParty' in root or '.git' in root:
            continue

        for filename in filenames:
            if filename.endswith('.cpp'):
                filepath = os.path.join(root, filename)
                try:
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        if 'defineEditorAttribute' in content:
                            files.append(filepath)
                except Exception as e:
                    print(f"Error reading {filepath}: {e}", file=sys.stderr)

    return files


def extract_define_editor_attribute_body(content: str) -> Optional[str]:
    """Extract the body of defineEditorAttribute function."""
    # Find the function definition
    pattern = r'void\s+\w+::defineEditorAttribute\s*\([^)]*\)\s*{([^}]*(?:{[^}]*}[^}]*)*?)}'

    match = re.search(pattern, content, re.DOTALL)
    if match:
        return match.group(1)
    return None


def analyze_slider_attributes(body: str) -> List[AttributePattern]:
    """Analyze slider attribute patterns (PdmUiDoubleSliderEditorAttribute, PdmUiSliderEditorAttribute)."""
    patterns = []

    # Pattern for slider attributes
    slider_pattern = r'(?:caf::)?PdmUi(?:Double)?SliderEditorAttribute\*\s+(\w+)\s*=\s*dynamic_cast<(?:caf::)?PdmUi(?:Double)?SliderEditorAttribute\*>\s*\(\s*attribute\s*\)'

    matches = re.finditer(slider_pattern, body)

    for match in matches:
        attr_var = match.group(1)

        # Find all attribute assignments for this variable
        # Look for patterns like: myAttr->m_minimum = value;
        attr_assignments = {}

        # Find the block where this attribute is used
        start_pos = match.end()
        # Find corresponding if block
        if_pattern = rf'if\s*\(\s*{attr_var}\s*\)'
        if_match = re.search(if_pattern, body[start_pos:])

        if if_match:
            # Extract the if block
            block_start = start_pos + if_match.end()
            # Simple brace matching (won't handle all cases but good enough for most)
            brace_count = 0
            block_end = block_start
            in_block = False

            for i, char in enumerate(body[block_start:], start=block_start):
                if char == '{':
                    in_block = True
                    brace_count += 1
                elif char == '}':
                    brace_count -= 1
                    if brace_count == 0 and in_block:
                        block_end = i
                        break

            if block_end > block_start:
                block = body[block_start:block_end]

                # Find attribute assignments
                assign_pattern = rf'{attr_var}->(\w+)\s*=\s*([^;]+);'
                for assign_match in re.finditer(assign_pattern, block):
                    attr_name = assign_match.group(1)
                    attr_value = assign_match.group(2).strip()
                    attr_assignments[attr_name] = attr_value

        # Now find which field this applies to
        # Look for: if ( field == &m_fieldName )
        field_pattern = r'if\s*\(\s*field\s*==\s*&(m_\w+)'
        field_matches = list(re.finditer(field_pattern, body))

        # Try to associate this slider with a field
        # This is simplified - real implementation would need better context tracking
        if attr_assignments:
            # Determine if static or dynamic
            is_static = all(
                is_constant_value(val) for val in attr_assignments.values()
            )

            patterns.append(AttributePattern(
                field_name="<detected_field>",  # Would need better parsing
                attribute_type="PdmUiDoubleSliderEditorAttribute",
                attributes=attr_assignments,
                is_static=is_static
            ))

    return patterns


def is_constant_value(value: str) -> bool:
    """Check if a value expression is a constant."""
    # Remove whitespace
    value = value.strip()

    # Check for numeric literals
    if re.match(r'^-?\d+\.?\d*$', value):
        return True

    # Check for simple expressions with only numbers
    if re.match(r'^[\d\.\+\-\*/\s()]+$', value):
        return True

    # If it contains function calls or member access, it's likely dynamic
    if '(' in value or '->' in value or '.' in value:
        return False

    return False


def analyze_button_attributes(body: str) -> List[AttributePattern]:
    """Analyze button attribute patterns (PdmUiPushButtonEditorAttribute)."""
    patterns = []

    # Pattern for button attributes
    button_pattern = r'(?:caf::)?PdmUiPushButtonEditorAttribute\*\s+(\w+)\s*=\s*dynamic_cast<(?:caf::)?PdmUiPushButtonEditorAttribute\*>\s*\(\s*attribute\s*\)'

    matches = re.finditer(button_pattern, body)

    for match in matches:
        attr_var = match.group(1)

        # Find button text assignments
        text_pattern = rf'{attr_var}->m_buttonText\s*=\s*([^;]+);'
        text_matches = list(re.finditer(text_pattern, body))

        if text_matches:
            # Check if there's a conditional (if/else for button text)
            has_conditional = len(text_matches) > 1

            for text_match in text_matches:
                button_text = text_match.group(1).strip()

                patterns.append(AttributePattern(
                    field_name="<detected_field>",
                    attribute_type="PdmUiPushButtonEditorAttribute",
                    attributes={"m_buttonText": button_text},
                    is_static=not has_conditional
                ))

    return patterns


def analyze_file(filepath: str) -> FileAnalysis:
    """Analyze a single file for migration opportunities."""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except Exception as e:
        return FileAnalysis(
            filepath=filepath,
            has_define_editor_attribute=False,
            patterns=[],
            needs_manual_review=True,
            manual_review_reason=f"Error reading file: {e}"
        )

    body = extract_define_editor_attribute_body(content)

    if not body:
        return FileAnalysis(
            filepath=filepath,
            has_define_editor_attribute=False,
            patterns=[],
            needs_manual_review=False
        )

    # Analyze different attribute types
    patterns = []
    patterns.extend(analyze_slider_attributes(body))
    patterns.extend(analyze_button_attributes(body))

    # Determine if needs manual review
    needs_manual = False
    reason = ""

    # Check for complex cases
    if 'dynamic_cast' in body and len(re.findall(r'dynamic_cast', body)) > 3:
        needs_manual = True
        reason = "Multiple attribute types, complex logic"

    # Check for custom types that can't be stored in QVariant
    if 'QFont' in body or 'QColor' in body or 'pickEventHandler' in body:
        needs_manual = True
        reason = "Contains complex attribute types (QFont, QColor, event handlers)"

    # Check for arrays/vectors
    if 'columnWidths' in body or 'std::vector' in body:
        needs_manual = True
        reason = "Contains array/vector attributes"

    return FileAnalysis(
        filepath=filepath,
        has_define_editor_attribute=True,
        patterns=patterns,
        needs_manual_review=needs_manual,
        manual_review_reason=reason
    )


def generate_migration_code(pattern: AttributePattern, config_name: str = "") -> List[str]:
    """Generate migration code for a pattern."""
    lines = []

    # Map attribute member names to setter method names and types
    attribute_mapping = {
        'm_minimum': ('setAttributeDouble', 'double'),
        'm_maximum': ('setAttributeDouble', 'double'),
        'm_sliderTickCount': ('setAttributeInt', 'int'),
        'm_buttonText': ('setAttributeString', 'string'),
        'm_placeholderText': ('setAttributeString', 'string'),
        'maximumWidth': ('setAttributeInt', 'int'),
    }

    for attr_name, attr_value in pattern.attributes.items():
        if attr_name in attribute_mapping:
            setter, _ = attribute_mapping[attr_name]

            # Convert member variable name to attribute name
            # m_minimum -> "minimum", m_buttonText -> "buttonText"
            clean_name = attr_name[2:] if attr_name.startswith('m_') else attr_name
            clean_name = clean_name[0].lower() + clean_name[1:] if clean_name else clean_name

            config_arg = f', "{config_name}"' if config_name else ''

            lines.append(
                f'{pattern.field_name}.uiCapability()->{setter}( "{clean_name}", {attr_value}{config_arg} );'
            )

    return lines


def print_report(analyses: List[FileAnalysis]):
    """Print a summary report."""
    print("=" * 80)
    print("MIGRATION ANALYSIS REPORT")
    print("=" * 80)
    print()

    total_files = len(analyses)
    files_with_attr = sum(1 for a in analyses if a.has_define_editor_attribute)
    files_needing_manual = sum(1 for a in analyses if a.needs_manual_review)
    files_auto_migrate = files_with_attr - files_needing_manual

    print(f"Total files analyzed: {total_files}")
    print(f"Files with defineEditorAttribute: {files_with_attr}")
    print(f"Files that can be auto-migrated: {files_auto_migrate}")
    print(f"Files needing manual review: {files_needing_manual}")
    print()

    print("=" * 80)
    print("FILES NEEDING MANUAL REVIEW")
    print("=" * 80)
    for analysis in analyses:
        if analysis.needs_manual_review and analysis.has_define_editor_attribute:
            print(f"\n{analysis.filepath}")
            print(f"  Reason: {analysis.manual_review_reason}")

    print()
    print("=" * 80)
    print("FILES READY FOR AUTO-MIGRATION")
    print("=" * 80)
    for analysis in analyses:
        if not analysis.needs_manual_review and analysis.has_define_editor_attribute and analysis.patterns:
            print(f"\n{analysis.filepath}")
            print(f"  Patterns detected: {len(analysis.patterns)}")
            for i, pattern in enumerate(analysis.patterns, 1):
                print(f"    {i}. {pattern.attribute_type}: {len(pattern.attributes)} attributes")


def main():
    """Main entry point."""
    if len(sys.argv) > 1:
        base_path = sys.argv[1]
    else:
        base_path = os.getcwd()

    print(f"Scanning for files in: {base_path}")
    print()

    # Find files
    files = find_files_with_define_editor_attribute(base_path)
    print(f"Found {len(files)} files with defineEditorAttribute")
    print()

    # Analyze files
    print("Analyzing files...")
    analyses = []
    for i, filepath in enumerate(files, 1):
        if i % 10 == 0:
            print(f"  Analyzed {i}/{len(files)} files...", end='\r')
        analysis = analyze_file(filepath)
        analyses.append(analysis)

    print(f"  Analyzed {len(files)}/{len(files)} files...done!")
    print()

    # Print report
    print_report(analyses)

    print()
    print("=" * 80)
    print("NEXT STEPS")
    print("=" * 80)
    print()
    print("1. Review files marked for manual review")
    print("2. For auto-migratable files, the script can generate migration patches")
    print("3. Run with --generate-patches to create migration code")


if __name__ == '__main__':
    main()
