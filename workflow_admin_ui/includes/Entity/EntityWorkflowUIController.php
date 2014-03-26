<?php

/**
 * @file
 * Contains workflow_admin_ui\includes\Entity\EntityWorkflowUIController.
 */

class EntityWorkflowUIController extends EntityDefaultUIController {
  /**
   * Provides definitions for implementing hook_menu().
   */
  public function hook_menu() {
    $items = parent::hook_menu();  

    // Set this on the object so classes that extend hook_menu() can use it.
    $id_count = count(explode('/', $this->path));
    $wildcard = isset($this->entityInfo['admin ui']['menu wildcard']) ? $this->entityInfo['admin ui']['menu wildcard'] : '%entity_object';
    $plural_label = isset($this->entityInfo['plural label']) ? $this->entityInfo['plural label'] : $this->entityInfo['label'] . 's';
    $entityType = $this->entityInfo['entity class'];

    $item = array(
      'file' => 'workflow_admin_ui.pages.inc',
      'file path' => 'workflow/workflow_admin_ui',
      'file' => $this->entityInfo['admin ui']['file'],
      'file path' => isset($this->entityInfo['admin ui']['file path']) ? $this->entityInfo['admin ui']['file path'] : drupal_get_path('module', $this->entityInfo['module']),
      'access arguments' => array('administer workflow'),
      // 'type' => MENU_CALLBACK,
      'type' => MENU_LOCAL_TASK,
    );

    $items[$this->path . '/manage/' . $wildcard . '/states'] = $item + array(
      'title' => 'States',
      'page callback' => 'drupal_get_form',
      'page arguments' => array('workflow_admin_ui_states_form', $id_count + 1, $id_count + 2),
    );

    $items[$this->path . '/manage/' . $wildcard . '/transitions'] = $item + array(
      'title' => 'Transitions',
      'page callback' => 'drupal_get_form',
      'page arguments' => array('workflow_admin_ui_transitions_form', $id_count + 1, $id_count + 2),
    );

    $items[$this->path . '/manage/' . $wildcard . '/permissions'] = $item + array(
      'title' => 'Permission summary',
      'page callback' => 'workflow_admin_ui_view_permissions_form',
      'page arguments' => array($id_count + 1, $id_count + 2),
      // @todo: convert to drupal_get_form('workflow_admin_ui_view_permissions_form');
      // 'page callback' => 'drupal_get_form',
      // 'page arguments' => array('workflow_admin_ui_view_permissions_form', $id_count + 1, $id_count + 2),
      'weight' => 1,
    );

    return $items;
  }

  protected function operationCount() {
    // Add more then enough colspan.
    return parent::operationCount() + 8;
  }

  public function overviewForm($form, &$form_state) {
    // Add table and pager.
    $form = parent::overviewForm($form, $form_state);

    // Allow modules to insert their own action links to the 'table', like cleanup module.
    $top_actions = module_invoke_all('workflow_operations', 'top_actions', NULL);

    // Allow modules to insert their own workflow operations.
    foreach ($form['table']['#rows'] as &$row ) {
      $url = $row[0]['data']['#url'];
      $workflow = $url['options']['entity'];
      foreach($actions = module_invoke_all('workflow_operations', 'workflow', $workflow) as $action) {
        $action['attributes'] = isset($action['attributes']) ? $action['attributes'] : array();
        $row[] = l(strtolower($action['title']), $action['href'], $action['attributes']);
      }
    }

    // @todo: add these top actions next to the core 'Add workflow' action.
    $top_actions_args = array(
      'links' => $top_actions,
      'attributes' => array('class' => array('inline', 'action-links')),
    );

    $form['action-links'] = array(
      '#type' => 'markup',
      '#markup' => theme('links', $top_actions_args),
      '#weight' => -1,
    );

    if (module_exists('workflownode')) {
      // Append the type_map form, changing the form by reference. 
      // The 'type_map' form is only valid for Workflow Node API.
      module_load_include('inc', 'workflow_admin_ui', 'workflow_admin_ui.type_map.page');
      workflow_admin_ui_type_map_form($form);
    }

    // Add a submit button. The submit functions are added in the sub-forms.
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
      '#weight' => 100,
    );

    return $form;
  }
}
