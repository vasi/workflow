<?php

/**
 * @file
 * Contains test functions.
 */

/*
 * Test functions.
 *
 * To test caches, load same object twice in a page (see d.o. issue #1572466).
 */
function workflow_test_entity_workflow_crud() {
  $ws = workflow_load_multiple();
  $count1 = count($ws);
  $workflow = workflow_create('test' . REQUEST_TIME);

  $workflow->save();

  // Test cache: $w3 must be OK, too.
  $w2 = workflow_load($workflow->wid);
  $w3 = workflow_load($workflow->wid);
  if ($w2 != $w3) {
    // error.
  }


  // Test Cache: number of workflows must be OK.
  $ws = workflow_load_multiple();
  $count2 = count($ws);

  $workflow->delete();
  $ws = workflow_load_multiple();
  $count3 = count($ws);

  if($count1 === $count3 && ($count2 - $count1) == 1) {
    drupal_set_message('workflow->create/save/delete OK');
  }
  else  {
    drupal_set_message('workflow->create/save/delete: error');
  }


  $workflow = workflow_create('test' . REQUEST_TIME);
  $s1 = $workflow->createState('first');
  $s2 = $workflow->createState('second');
  $s3 = $workflow->createState('third');
  $s1->save();
  $s2->save();
  $s2->save();
  $s3->save();
}

function workflow_test_entity1() {
  $workflow = NULL;

  // Create a workflow.
  dpm('--- Create Workflow ---');
  dpm($workflow, '--- test workflow_create() pre');
  $workflow = workflow_create('test' . REQUEST_TIME);
  dpm($workflow, '--- test workflow_create() pre_save');
  $workflow->save();
  dpm($workflow, '--- test workflow_create() post_save');

  // Create States for the workflow.
  dpm('--- Create States ---');
  dpm($workflow->getStates(TRUE), '--- test getStates() pre');
  dpm($workflow, '--- test getStates() pre');
  $s1 = $workflow->createState('first');
  dpm($workflow->getStates(TRUE), '--- test getStates() post s1');
  dpm($workflow, '--- test getStates() post s1');
  $s2 = $workflow->createState('second');
  $s3 = $workflow->createState('third');
//  $s1->save();
//  $s2->save();
//  $s3->save();
  dpm($workflow->getStates(TRUE), '--- test getStates() post');
  dpm($workflow, '--- test getStates() post');
  dpm($s3->getWorkflow(), '--- test getStates() post');

  // Create Transitions for the workflow.
  dpm('--- Create Transitions ---');
  dpm('--- Create Transition s1->s2 ---');
  dpm($workflow->getTransitions(), '--- test getTransitions() pre');
  $roles = array(-1, 5);
  $t1 = $workflow->createTransition($s1->sid, $s2->sid);
  $t1->label = 'transitions t1';
  $t1->roles = $roles;
  // The transition was initially saved in createState,
  // but without roles and name.
  $t1->save();
  dpm($t1, '-- test show new Transition');
  dpm($workflow->getTransitions(), '-- test getTransitions() post 1');

  dpm('--- Create Transition s1->s2 ---');
  dpm($workflow->getTransitions(), '--- test getTransitions() pre');
  $roles = array(-1, 6);
  $t2 = $workflow->createTransition($s2->sid, $s3->sid);
  $t2->label = 'transitions t2';
  $t2->roles = $roles;
  // The transition was initially saved in createState,
  // but without roles and name.
  $t2->save();
  dpm($t2, '-- test show new Transition');
  dpm($workflow->getTransitions(), '-- test getTransitions() post 2');

  dpm('--- Show resulting Workflow ---');
  dpm($workflow);
  dpm('todo: Test if workflow, states and transitions are properly shown in Admin UI.');
  dpm('--- Show resulting Workflow after workflow_load---');
  // Refresh the workflow, and show again.
  $workflow2 = workflow_load($workflow->wid);
  dpm($workflow2);

//  dpm($t->uri());
//  dpm($t->label());

  // Remove the workflow and its components.
  dpm('--- Delete resulting Workflow ---');
  $workflow->delete();
  dpm($workflow);
  dpm('todo: Test if workflow, states and transitions are properly deleted.');
}

function workflow_test_entity2() {
  $w = workflow_load('test');
  dpm($w);
  dpm($w->getTransitions());
  return;

  $ts = entity_load('WorkflowConfigTransition', array(6));
  $t = reset($ts);
  $t->roles += array(3 => 3);
  entity_save('WorkflowConfigTransition', $t);
}

function workflow_test_entity3() {
  $workflow = workflow_load_single(1);
  dpm($workflow->getStates());
dpm($workflow->loadTransitions());
//  $t1 = $workflow->createTransition($s1->sid, $s2->sid);
//  $t1->save();
//  dpm($t1);
//  $t2 = $workflow->createTransition('third', 'second');
//  $t2->save();
//  dpm($t2);
//  dpm($t->uri());
//  dpm($t->label());
// $workflow->delete();

//  $ts = entity_load('WorkflowConfigTransition', array(6));
//  $t = reset($ts);
//  $t->roles += array(3 => 3);
//  entity_save('WorkflowConfigTransition', $t);
}

function workflow_test_entity_allowable() {
  $workflow = workflow_create('workflow_test_'.REQUEST_TIME);
  $workflow->save();
  $s1 = $workflow->createState('to be opened');
  $s2 = $workflow->createState('open');
  $s3 = $workflow->createState('closed');
  $s1->save();
  $s2->save();
  $s3->save();
  $roles = array(1, 2); // anon, auth user.
  $t1 = $t = $workflow->createTransition(WORKFLOW_CREATION_STATE_NAME, 'to be opened');
  $t->roles = $roles;
  $t->save();
  $t2 = $t = $workflow->createTransition('to be opened', 'open');
  $t->roles = $roles;
  $t->save();
  $t3 = $t = $workflow->createTransition('open', 'closed');
  $t->roles = $roles;
DPM($t3);
  $t->save();
DPM($t3);
  $t4 = $t = $workflow->createTransition('closed', 'open');
  $t->roles = $roles;
  $t->save();

  dpm($s2->getOptions('', NULL));
  // returns  TRUE if the role is allowed to do the transition.
  $rc = $t3->isAllowed($role = 2);
  dpm($t3);
  dpm('this result must be TRUE: ' . (int) $rc);
  $rc = $t3->isAllowed($role = 3);
  dpm('this result must be FALSE: ' . (int) $rc);

  $rc = $workflow->GetTransitionsBySid($s3->sid);
  dpm($rc, 'allowed transitions from ' . $s3->label());

  $ts = $workflow->getTransitions();
  dpm($ts);
  $ts = $workflow->getTransitionsBySid($s3->sid);
  dpm($ts);

  $workflow->delete();
}


function _workflow_test_metadata() {
  // Fnd the first, arbitrary workflow.
  $workflows = workflow_load_multiple();
  $workflow = reset($workflows);
dpm($workflow);

  // Add a wrapper;
  $wrapper = entity_metadata_wrapper('Workflow', $workflow);
dpm($wrapper);

  // Get properties of the Workflow.
dpm(  $wrapper->wid->label(). ' ' . $wrapper->wid->value()  );
dpm(  $wrapper->name->label(). ' ' . $wrapper->name->value()  );
dpm(  $wrapper->label->label(). ' ' . $wrapper->label->value()  );
dpm(  $wrapper->status->value()  );
dpm(  $wrapper->tab_roles->value()  );
//dpm(  $wrapper->states->value()  );
dpm(  $wrapper->states->optionsList()  );
//dpm(  $wrapper->transitions->value()  );
dpm(  $wrapper->options->optionsList()  );
dpm('---- end of Workflow getters ---');
  // Set properties of the Workflow.
  //dpm(  $wrapper->label->set('ssdfdss')  );
  // ...

  // Get properties of States.
  $states = $workflow->getStates();
  $state = reset($states);
  $state_wrapper = entity_metadata_wrapper('WorkflowState', $state);
dpm($states);
dpm(  $state_wrapper->sid->value()  );
dpm(  $state_wrapper->wid->value()  );
dpm(  $state_wrapper->weight->value()  );
dpm(  $state_wrapper->state->value()  );
dpm(  $state_wrapper->status->value()  );

  //dpm(  $wrapper->author->mail->value()  );
  //dpm(  $wrapper->title->value(array('sanitize' => TRUE))  );
}
