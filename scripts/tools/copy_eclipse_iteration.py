#!/usr/bin/env python3
"""
Script to copy Eclipse SMSPEC and UNSMRY files from a source iteration
to a destination iteration across all realizations in an ensemble.

Usage:
    python copy_eclipse_iteration.py --base_path <path> --source_iter <iter> --dest_iter <iter>
"""

import os
import shutil
import argparse
from pathlib import Path
from typing import List, Dict
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


class EclipseIterationCopier:
    """Class to handle copying Eclipse files between iterations."""
    
    def __init__(self, base_path: str):
        self.base_path = Path(base_path)
        if not self.base_path.exists():
            raise ValueError(f"Base path does not exist: {base_path}")
        
        logger.info(f"Initialized copier for base path: {self.base_path}")
    
    def find_realizations(self) -> List[Path]:
        """Find all realization directories in the base path."""
        realizations = []
        for item in self.base_path.iterdir():
            if item.is_dir() and item.name.startswith('realization-'):
                realizations.append(item)
        
        # Sort by realization number
        realizations.sort(key=lambda x: int(x.name.split('-')[1]))
        logger.info(f"Found {len(realizations)} realizations")
        return realizations
    
    def get_eclipse_model_path(self, realization_path: Path, iteration: str) -> Path:
        """Get the eclipse/model path for a specific realization/iteration."""
        return realization_path / iteration / "eclipse" / "model"
    
    def find_eclipse_files(self, model_path: Path) -> Dict[str, List[Path]]:
        """Find SMSPEC and UNSMRY files in a model directory."""
        files = {
            'smspec': [],
            'unsmry': []
        }
        
        if not model_path.exists():
            logger.warning(f"Model path does not exist: {model_path}")
            return files
        
        # Find SMSPEC files
        smspec_files = list(model_path.glob("*.SMSPEC"))
        files['smspec'] = smspec_files
        
        # Find UNSMRY files
        unsmry_files = list(model_path.glob("*.UNSMRY"))
        files['unsmry'] = unsmry_files
        
        logger.debug(f"Found {len(smspec_files)} SMSPEC and {len(unsmry_files)} UNSMRY files in {model_path}")
        
        return files
    
    def copy_files(self, source_files: List[Path], dest_dir: Path) -> List[Path]:
        """Copy files to destination directory."""
        copied_files = []
        
        # Create destination directory if it doesn't exist
        dest_dir.mkdir(parents=True, exist_ok=True)
        
        for source_file in source_files:
            dest_file = dest_dir / source_file.name
            
            try:
                shutil.copy2(source_file, dest_file)
                copied_files.append(dest_file)
                logger.debug(f"Copied: {source_file} -> {dest_file}")
                
            except Exception as e:
                logger.error(f"Error copying {source_file} to {dest_file}: {e}")
        
        return copied_files
    
    def copy_iteration(self, source_iter: str, dest_iter: str, dry_run: bool = False) -> Dict[str, Dict]:
        """Copy files from source iteration to destination iteration for all realizations."""
        results = {}
        realizations = self.find_realizations()
        
        logger.info(f"{'DRY RUN: ' if dry_run else ''}Copying from '{source_iter}' to '{dest_iter}' for {len(realizations)} realizations")
        
        total_copied = 0
        total_errors = 0
        
        for realization_path in realizations:
            realization_name = realization_path.name
            logger.info(f"Processing {realization_name}")
            
            # Get source and destination paths
            source_model_path = self.get_eclipse_model_path(realization_path, source_iter)
            dest_model_path = self.get_eclipse_model_path(realization_path, dest_iter)
            
            # Find source files
            source_files = self.find_eclipse_files(source_model_path)
            
            realization_data = {
                'source_path': source_model_path,
                'dest_path': dest_model_path,
                'source_files': source_files,
                'copied_files': {'smspec': [], 'unsmry': []},
                'errors': []
            }
            
            if not source_model_path.exists():
                error_msg = f"Source path does not exist: {source_model_path}"
                logger.warning(error_msg)
                realization_data['errors'].append(error_msg)
                total_errors += 1
            else:
                # Copy SMSPEC files
                if not dry_run and source_files['smspec']:
                    copied_smspec = self.copy_files(source_files['smspec'], dest_model_path)
                    realization_data['copied_files']['smspec'] = copied_smspec
                    total_copied += len(copied_smspec)
                
                # Copy UNSMRY files
                if not dry_run and source_files['unsmry']:
                    copied_unsmry = self.copy_files(source_files['unsmry'], dest_model_path)
                    realization_data['copied_files']['unsmry'] = copied_unsmry
                    total_copied += len(copied_unsmry)
                
                if dry_run:
                    # Just count what would be copied
                    would_copy = len(source_files['smspec']) + len(source_files['unsmry'])
                    total_copied += would_copy
                    logger.info(f"  Would copy {len(source_files['smspec'])} SMSPEC and {len(source_files['unsmry'])} UNSMRY files")
                else:
                    copied_count = len(realization_data['copied_files']['smspec']) + len(realization_data['copied_files']['unsmry'])
                    logger.info(f"  Copied {copied_count} files")
            
            results[realization_name] = realization_data
        
        logger.info(f"{'DRY RUN: ' if dry_run else ''}Total files {'would be ' if dry_run else ''}copied: {total_copied}")
        if total_errors > 0:
            logger.warning(f"Total errors: {total_errors}")
        
        return results
    
    def print_summary(self, results: Dict[str, Dict], source_iter: str, dest_iter: str, dry_run: bool = False):
        """Print a summary of the copy operation."""
        print(f"\n{'='*60}")
        print(f"ECLIPSE ITERATION COPY {'(DRY RUN) ' if dry_run else ''}SUMMARY")
        print(f"{'='*60}")
        print(f"Base path: {self.base_path}")
        print(f"Source iteration: {source_iter}")
        print(f"Destination iteration: {dest_iter}")
        print(f"Total realizations processed: {len(results)}")
        
        total_smspec_copied = 0
        total_unsmry_copied = 0
        total_errors = 0
        
        for realization_name, data in results.items():
            smspec_count = len(data['copied_files']['smspec']) if not dry_run else len(data['source_files']['smspec'])
            unsmry_count = len(data['copied_files']['unsmry']) if not dry_run else len(data['source_files']['unsmry'])
            error_count = len(data['errors'])
            
            total_smspec_copied += smspec_count
            total_unsmry_copied += unsmry_count
            total_errors += error_count
            
            if smspec_count > 0 or unsmry_count > 0 or error_count > 0:
                print(f"\n{realization_name}:")
                if dry_run:
                    print(f"  Would copy SMSPEC files: {smspec_count}")
                    print(f"  Would copy UNSMRY files: {unsmry_count}")
                else:
                    print(f"  Copied SMSPEC files: {smspec_count}")
                    print(f"  Copied UNSMRY files: {unsmry_count}")
                
                if error_count > 0:
                    print(f"  Errors: {error_count}")
                    for error in data['errors']:
                        print(f"    - {error}")
        
        print(f"\n{'='*60}")
        print(f"TOTALS:")
        if dry_run:
            print(f"  SMSPEC files to copy: {total_smspec_copied}")
            print(f"  UNSMRY files to copy: {total_unsmry_copied}")
        else:
            print(f"  SMSPEC files copied: {total_smspec_copied}")
            print(f"  UNSMRY files copied: {total_unsmry_copied}")
        print(f"  Total errors: {total_errors}")
        print(f"{'='*60}")


def main():
    parser = argparse.ArgumentParser(description='Copy Eclipse files from source to destination iteration')
    parser.add_argument('--base_path', type=str, required=True,
                       help='Base path containing realization directories')
    parser.add_argument('--source_iter', type=str, required=True,
                       help='Source iteration name (e.g., iter-0, iter-1, base_pred)')
    parser.add_argument('--dest_iter', type=str, required=True,
                       help='Destination iteration name (e.g., iter-0, iter-1, base_pred)')
    parser.add_argument('--dry_run', action='store_true',
                       help='Show what would be copied without actually copying')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Enable verbose logging')
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    if args.source_iter == args.dest_iter:
        logger.error("Source and destination iterations cannot be the same")
        return None
    
    try:
        # Create copier instance
        copier = EclipseIterationCopier(args.base_path)
        
        # Copy files between iterations
        results = copier.copy_iteration(args.source_iter, args.dest_iter, args.dry_run)
        
        # Print summary
        copier.print_summary(results, args.source_iter, args.dest_iter, args.dry_run)
        
        return results
        
    except Exception as e:
        logger.error(f"Error copying iteration: {e}")
        return None


if __name__ == "__main__":
    main()