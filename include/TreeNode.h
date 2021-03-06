/**
 * @file TreeNode.h
 * This file defines the mcts::UCTreeNode class.
 */
#ifndef MCTS_TREENODE_H
#define MCTS_TREENODE_H

#include <cmath>
#include <cassert>
#include <limits>
#include <stack>
#include <iostream>
#include <eigen3/Eigen/Dense>

/**
 * Namespace for all public functions and types defined in the MCTS library.
 */
namespace mcts {

/**
 * Used to generate small random numbers to break ties when selecting actions.
 */
const double EPSILON = 1e-6;

/**
 * Simple uniform random number generator.
 * This provides a default type for generating random numbers for
 * performing Monte Carlo Tree Search.
 */
struct SimpleURand
{
   /**
    * Generates uniform random numbers in the range [0,1).
    */
   double operator()()
   {
      return static_cast<double>(rand()%RAND_MAX)/RAND_MAX;
   }
};

    
/** UCTreeNode CLASS
 *________________________________________________________________________________
 * Represents a node in a UCT tree. This provides the main data structure and
 * implementation of the UCT (Upper Confidence Tree) algorithm.
 * @tparam URand class used to generate uniform random numbers in range [0,1).
 * This is used internally during selection and rollout.
 */
template<class URand=SimpleURand> class UCTreeNode
{
private: 

   /**
    * Array of pointers (one per action) to all children of this node.
    */
   UCTreeNode* vpChildren_i[2];

   /**
    * True iff this is a leaf node with no children. If this variable is true
    * then all elements of the UCTreeNode::vpChildren_i array should be null,
    * otherwise they should all point to valid objects.
    */
   bool isLeaf_i;

   /**
    * Counts the number of time this node has been visited.
    */
   double nVisits_i;
   
   /**
    * Sum of all values received each time this node has been visited.
    */
   double totValue_i;

   /**
    * Uniform random number generated used for selection and rollout.
    */
   URand rand_i;
    
   /**
    * The centre point of the space that the node represents
    */
   Eigen::Vector3d centre_i;
    
   /*
    * difference between centre and maximum in each axis
    */
   Eigen::Vector3d range_i;

   /**
    * Selects the next action to explore using UCB.
    * @pre This must not be a leaf node.
    * @returns the index of the selected child's action
    */
   int selectBranch()
   {
      //***********************************************************************
      // Ensure preconditions are meet: can't select a child, if this is a
      // leaf node.
      //***********************************************************************
      assert(!isLeaf_i);

      //***********************************************************************
      // Initialise selected node to null, and the best value found so far
      // to its lowest possible value.
      //***********************************************************************
      int selected = 0;
      double bestValue = -std::numeric_limits<double>::max();

      //***********************************************************************
      // For each child node
      //***********************************************************************
      for(int k=0; k<2; ++k)
      {
         //********************************************************************
         // Sanity check that the current child is not null (this should
         // never happen for non leaf nodes).
         //********************************************************************
         UCTreeNode* pCur = vpChildren_i[k]; // ptr to current child node
         assert(0!=pCur);

         //********************************************************************
         // Calculate the UCT value of the current child
         //********************************************************************
         double uctValue = pCur->totValue_i / (pCur->nVisits_i + EPSILON) +
            std::sqrt(std::log(nVisits_i+1) / (pCur->nVisits_i + EPSILON));

         //********************************************************************
         // Add a small random value to break ties
         //********************************************************************
         uctValue += rand_i()*EPSILON;

         //********************************************************************
         // If the current child is the best one encounted so far, make it
         // the selected node, and store its value for later comparsion.
         //********************************************************************
         if (uctValue >= bestValue)
         {
            selected = k;
            bestValue = uctValue;
         }

      } // for loop

      //***********************************************************************
      // Return the index of the selected action.
      //***********************************************************************
      return selected;

   } // selectBranch

    
   /**
    * If this is a leaf node, expands the tree by initalising this node's
    * children. If this is not a leaf node, no action is taken.
    * @post If this is a leaf node, a new child node will be constructed for
    * each possible action.
    */
   void expand()
   {
       // If this is not a leaf node, then we're done. There is nothing to do.
       if(!isLeaf_i)
       {
           return;
       }

       // Otherwise, we set the leaf flag to false (to indicate that this is
       // no longer a leaf node)
       isLeaf_i = false;
       
       // construct a new child for each action.
       axis = rand() % 3; // generate an int from 0 to 2 to randomly select a axis to expand
       
       // init the in_range and in_centre buffer
       Eigen::Vector3d range  = range_i(axis)/2.0; // buffer for in_range for children (half of the range of the parent)
       Eigen::Vector3d centre = centre_i;          // buffer for in_centre for children
       
       // first child
       centre(axis) = centre_i(axis) + range(axis);
       vpChildren_i[0] = new UCTreeNode(centre, range);
       
       // second child
       centre(axis) = centre_i(axis) - range(axis);
       vpChildren_i[1] = new UCTreeNode(centre, range);
   } // expand 

    
   /**
    * Returns an estimated value for a leaf node using the rollout policy.
    * @param[in] mdp A number generator which returns a reward for a given
    * action.
    * @tparam[in] Generator Functor type which overloads the () operator by
    * returning a random reward for a given action index.
    * @return the estimated rollout value for \c node.
    */
   template<class Generator> double rollOut(Generator mdp)
   {
      // Select a random position within the space that the node represents.
       Eigen::Vector3d action;
       for(int i=0; i<3; i++){
           action(i) = 2.0 * rand_i() * range_i(i) - range_i(i) + centre_i(i);
       }
      
      // Generate reward
      double totReward = 0.0;
      totReward = mdp(action);
      return totReward;
   } // rollout

    
   /**
    * Updates the statistics for this node for a given observed value.
    * @param[in] value the observed value.
    */
   void updateStats(double value)
   {
      nVisits_i++;         // increment the number of visits
      totValue_i += value; // update the total value for all visits
   }

    
    
    
public:

   /**
    * Construct a new UCTreeNode leaf node.
    * @param[in] inGamma discount factor for future rewards.
    * @param[in] inRand a uniform random number generated used for selection and
    * rollout.
    */
    UCTreeNode(Eigen::Vector3d in_centre, Eigen::Vector3d in_range, URand inRand=URand())
      : isLeaf_i(true), nVisits_i(0), totValue_i(0), centre_i(in_centre), range_i(in_range)
        rand_i(inRand)
   {
      //***********************************************************************
      // Since this is a leaf node with no children, all child pointers
      // should be null
      //***********************************************************************
      for(int k=0; k<2; ++k)
      {
         vpChildren_i[k] = 0;
      }

   }  // default constructor

   /**
    * Copy assignment. Performs a deep copy, including all children.
    * @param[in] tree the tree to copy.
    */
   UCTreeNode& operator=(const UCTreeNode& tree)
   {
      //***********************************************************************
      // Delete old children if necessary
      //***********************************************************************
      if(!isLeaf_i)
      {
         for(int k=0; k<2; ++k)
         {
            assert(0!=vpChildren_i[k]);
            delete vpChildren_i[k];
            vpChildren_i[k]=0;

         } // for loop

      } // if statement

      //***********************************************************************
      // Set all scalar members
      //***********************************************************************
      isLeaf_i = tree.isLeaf_i;
      nVisits_i = tree.nVisits_i;
      totValue_i = tree.totValue_i;
      rand_i = tree.rand_i;

      //***********************************************************************
      // Copy new children if necessary
      //***********************************************************************
      if(!isLeaf_i)
      {
         for(int k=0; k<2; ++k)
         {
            assert(0!=tree.vpChildren_i[k]);
            assert(0==vpChildren_i[k]);
            vpChildren_i[k] = new UCTreeNode(*tree.vpChildren[k]);

         } // for loop

      } // if statement

   } // operator=

   /**
    * Returns true if this is a leaf node with no children.
    */
   bool isLeaf() const
   {
      return isLeaf_i;
   }

   /**
    * Performs one iteration of the MCTS algorithm, taking this to be the root
    * node. (This function is embedded in all nodes in the tree, however, only
    * the function of root node is called)(one iteration always starts from root
    * noot and ends at leaf node)
    
    * @param[in] mdp A number generator which returns a reward for a given
    * action.
    * @tparam[in] Generator Functor type which overloads the () operator by
    * returning a random reward for a given action index.
    * @post the depth of the current best path from this node to the top of
    * the tree will be expanded by one. The value of all nodes along this path
    * will also be updated.
    */
   template<class Generator> void iterate(Generator mdp)
   {
      //***********************************************************************
      // Create stack to hold the path visited on this iteration.
      // Initially this holds only the current node.
      //***********************************************************************
      std::stack<UCTreeNode*> visited;
      UCTreeNode* pCur = this;
      visited.push(this);

      //***********************************************************************
      // Transverse the highest value path from the current node, until
      // we hit a leaf node. We also record rewards for each action as we go
      // along.
      //***********************************************************************
      int action = 0; // next selected action
      double curReward = 0.0; // immediate reward for last action
      while (!pCur->isLeaf())
      {
         action = pCur->selectBranch();
         pCur = pCur->vpChildren_i[action];
         visited.push(pCur);
      }

      //***********************************************************************
      // Expand the current (leaf) node by depth 1, and select its best child.
      //***********************************************************************
      pCur->expand();
      action = pCur->selectBranch();
      pCur = pCur->vpChildren_i[action];
      visited.push(pCur);

      //***********************************************************************
      // Estimate the value of the new leaf node (expanded children) using the
      // rollout policy
      //***********************************************************************
      double value = rollOut(mdp);

      //***********************************************************************
      // Update the statistics for each node along the path using the
      // discounted value. 
      //***********************************************************************
      while(!visited.empty())
      {
         pCur = visited.top();       // get the current node in the path
         pCur->updateStats(value);   // update statistics
         visited.pop();              // remove the current node from the stack
      }

   } // iterate

   /**
    * Returns the current best action for the next step.
    * @returns the index of the best action.
    */
   int bestAction() 
   {
      //***********************************************************************
      // If this is a leaf node, we have no information to go on, so we
      // just return a random action.
      //***********************************************************************
      if(isLeaf_i)
      {
         return rand_i()*2;
      }

      //***********************************************************************
      // Initialise selected action, and the best value found so far
      // to its lowest possible value.
      //***********************************************************************
      int selected = 0;
      double bestValue = -std::numeric_limits<double>::max();

      //***********************************************************************
      // For each child node
      //***********************************************************************
      for(int k=0; k<2; ++k)
      {
         //********************************************************************
         // Sanity check that the current child is not null (this should
         // never happen for non leaf nodes).
         //********************************************************************
         UCTreeNode* pCur = vpChildren_i[k]; // ptr to current child node
         assert(0!=pCur);

         //********************************************************************
         // Calculate expected value
         //********************************************************************
         double expValue = pCur->totValue_i / (pCur->nVisits_i + EPSILON);

         //********************************************************************
         // Add a small random value to break ties
         //********************************************************************
         expValue += static_cast<double>(rand())*EPSILON/RAND_MAX;

         //********************************************************************
         // If the current child is the best one encounted so far, make it
         // the selected node, and store its value for later comparsion.
         //********************************************************************
         if (expValue >= bestValue)
         {
            selected = k;
            bestValue = expValue;
         }

      } // for loop

      //***********************************************************************
      // Return the index of the best value
      //***********************************************************************
      return selected;

   } // bestAction

   /**
    * Returns the expected value for this tree.
    */
   double vValue() const
   {
      return totValue_i/nVisits_i;
   }

   /**
    * Returns the Q-value for a given action.
    * @param[in] action the index of the action whose value should be returned.
    * @pre \c action must be between 0 and \c N_Branch (the number of actions).
    */
   double qValue(int action) const
   {
      assert(0<=action);
      assert(2>action);
      return vpChildren_i[action]->vValue();
   }

   /**
    * Counts the number of nodes in the tree with this node as its root.
    */
   int numOfNodes() const
   {
      //***********************************************************************
      // If this is a leaf node, then there is only one node in the tree
      //***********************************************************************
      if(isLeaf_i)
      {
         return 1;
      }

      //***********************************************************************
      // Otherwise, we have to count the number of nodes in each subtree
      //***********************************************************************
      int result = 1;
      for(int k=0; k<2; ++k)
      {
         result += vpChildren_i[k]->numOfNodes();
      }
      return result;

   } // numOfNodes

   /**
    * Returns the maximum depth of the tree with this node as its root.
    * The \c parentDepth parameter specifies the depth of the parent node
    * for recursion purposes. When called externally with this node as the
    * true root of the tree, it is usually sufficient to leave this parameter
    * with its default value of 0.
    * @param[in] parentDepth depth of parent node.
    */
   int maxDepth(int parentDepth=0) const
   {
      //***********************************************************************
      // If this is a leaf node, then the result is just the parent's depth
      // plus 1.
      //***********************************************************************
      if(isLeaf_i)
      {
         return parentDepth + 1;
      }

      //***********************************************************************
      // Otherwise, we return the maximum subtree depth plus 1.
      //***********************************************************************
      int maxDepth = 0;
      for(int k=0; k<2; ++k)
      {
         int curDepth = vpChildren_i[k]->maxDepth(parentDepth+1);
         if(maxDepth < curDepth)
         {
            maxDepth = curDepth;
         }
      }

      return maxDepth;

   } // maxDepth

   /**
    * Destructor deletes all child nodes in addition to this one.
    */
   virtual ~UCTreeNode()
   {

      if(isLeaf_i)
      {
         return;
      }

      for(int k=0; k<2; ++k)
      {
         if(0!=vpChildren_i[k])
         {
            delete vpChildren_i[k];
            vpChildren_i[k]=0;
         }
      }

   } // destructor 

}; // class UCTreeNode

/**
 * Produces a string representation of a treeNode for diagnostic purposes.
 * Basically, just prints the vValue and the qValue for each action.
 */
std::ostream& operator<<
(
 std::ostream& out,
 const UCTreeNode& tree
)
{

   //**************************************************************************
   // Print the V value
   //**************************************************************************
   out << "[V=" << tree.vValue();
   
   //**************************************************************************
   // If this is a leaf node, then we're done.
   //**************************************************************************
   if(tree.isLeaf())
   {
      out << ']';
      return out;
   }

   //**************************************************************************
   // Otherwise, also print the q values for each action.
   //**************************************************************************
   for(int k=0; k<2; ++k)
   {
      out << ",Q" << k << '=' << tree.qValue(k);
   }
   out << ']';

   return out;

} // operator <<

} // namespace mcts

#endif // MCTS_TREENODE_H
