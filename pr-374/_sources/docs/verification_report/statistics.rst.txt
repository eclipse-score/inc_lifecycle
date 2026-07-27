.. _life_statistics:

Verification Statistics
=======================

Overview
--------

.. needpie:: Requirements Status
   :labels: not valid, valid but not tested, valid and tested
   :colors: red, yellow, green

   type == 'comp_req' and status == 'invalid'
   type == 'comp_req' and testlink == '' and (status == 'valid' or status == 'invalid')
   type == 'comp_req' and testlink != '' and (status == 'valid' or status == 'invalid')

In Detail
---------

.. grid:: 2
   :class-container: score-grid

   .. grid-item-card::

      .. needpie:: Requirements marked as Valid
         :labels: not valid, valid
         :colors: red, orange, green

         type == 'comp_req' and status == 'invalid'
         type == 'comp_req' and status == 'valid'

   .. grid-item-card::

      .. needpie:: Requirements with Codelinks
         :labels: no codelink, with codelink
         :colors: red, green

         type == 'comp_req' and source_code_link == ''
         type == 'comp_req' and source_code_link != ''

   .. grid-item-card::

      .. needpie:: Test Results
         :labels: passed, failed, skipped
         :colors: green, red, orange

         type == 'testcase' and result == 'passed'
         type == 'testcase' and result == 'failed'
         type == 'testcase' and result == 'skipped'

.. grid:: 2

   .. grid-item-card::

      Failed Tests

      *Hint: This table should be empty. Before a PR can be merged all tests have to be successful.*

      .. needtable:: FAILED TESTS
         :filter: result == "failed"
         :tags: TEST
         :columns: name as "testcase";result;fully_verifies;partially_verifies;test_type;derivation_technique;id as "link"

   .. grid-item-card::

      Skipped / Disabled Tests

      .. needtable:: SKIPPED/DISABLED TESTS
         :filter: result != "failed" and result != "passed"
         :tags: TEST
         :columns: name as "testcase";result;fully_verifies;partially_verifies;test_type;derivation_technique;id as "link"




All passed Tests
-----------------

.. needtable:: SUCCESSFUL TESTS
   :filter: result == "passed"
   :tags: TEST
   :columns: name as "testcase";result;fully_verifies;partially_verifies;test_type;derivation_technique;id as "link"


Details About Testcases
------------------------

.. needpie:: Test Types Used In Testcases
   :labels: static-code-analysis, structural-statement-coverage, structural-branch-coverage, walkthrough, inspection, interface-test, requirements-based, resource-usage, control-flow-analysis, data-flow-analysis, fault-injection, struct-func-cov, struct-call-cov
   :legend:

   type == 'testcase' and test_type == 'static-code-analysis'
   type == 'testcase' and test_type == 'structural-statement-coverage'
   type == 'testcase' and test_type == 'structural-branch-coverage'
   type == 'testcase' and test_type == 'walkthrough'
   type == 'testcase' and test_type == 'inspection'
   type == 'testcase' and test_type == 'interface-test'
   type == 'testcase' and test_type == 'requirements-based'
   type == 'testcase' and test_type == 'resource-usage'
   type == 'testcase' and test_type == 'control-flow-analysis'
   type == 'testcase' and test_type == 'data-flow-analysis'
   type == 'testcase' and test_type == 'fault-injection'
   type == 'testcase' and test_type == 'struct-func-cov'
   type == 'testcase' and test_type == 'struct-call-cov'


.. needpie:: Derivation Techniques Used In Testcases
   :labels: requirements-analysis, boundary-values, equivalence-classes, fuzz-testing, error-guessing, explorative-testing
   :legend:

   type == 'testcase' and derivation_technique == 'requirements-analysis'
   type == 'testcase' and derivation_technique == 'boundary-values'
   type == 'testcase' and derivation_technique == 'equivalence-classes'
   type == 'testcase' and derivation_technique == 'fuzz-testing'
   type == 'testcase' and derivation_technique == 'error-guessing'
   type == 'testcase' and derivation_technique == 'explorative-testing'
